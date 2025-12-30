section .text
global asm_spectral_sharpen
global asm_calculate_entropy_fast
global asm_fft_butterfly

; =============================================================================
; void asm_spectral_sharpen(complex_fixed_t* buffer, uint32_t size, int32_t threshold)
; 32-bit x86 implementation
; Arguments (cdecl/stdcall - stack):
; [ebp + 8]  = buffer
; [ebp + 12] = size
; [ebp + 16] = threshold
; =============================================================================
asm_spectral_sharpen:
    push ebp
    mov ebp, esp
    push ebx
    push esi
    push edi
    
    mov esi, [ebp + 8]      ; buffer
    mov ecx, [ebp + 12]     ; size (loop counter)
    mov edx, [ebp + 16]     ; threshold
    
    ; We process from index 0 to size-1. 
    ; Let's use a register for current offset or pointer.
    ; ecx is size. Let's loop from 0 to size.
    
    xor edi, edi            ; index = 0
    
.loop:
    cmp edi, ecx
    jge .end
    
    ; Load real and imag
    mov eax, [esi + edi*8]      ; real
    mov ebx, [esi + edi*8 + 4]  ; imag
    
    ; Compute magnitude squared: real^2 + imag^2
    ; Mul: edx:eax <- eax * source
    
    push edx        ; Save threshold
    push ecx        ; Save size
    
    mov ecx, eax    ; Save real
    imul eax, eax   ; real^2 (lower 32-bit in eax, upper in edx - ignored for approx)
    ; Note: For true 16.16 fixed point, squares can be large. 
    ; But here we assume we stay within range for this "approx" check or stick to 32 bit logic for simplicity in kernel.
    
    mov [ebp - 4], eax ; Store real^2 (using stack scratch space, simplified)
                       ; Actually let's use registers if we can. 
    mov edx, ebx
    imul edx, edx      ; imag^2
    add eax, edx       ; mag^2
    
    ; Restore threshold to comparison register (we pushed it)
    ; But wait, we pushed edx (threshold) and ecx (size).
    ; Stack is: [threshold] [size] [ret] [ebp]
    
    pop ecx            ; restore size
    pop edx            ; restore threshold
    
    ; Compare mag^2 vs threshold^2
    push eax           ; Save mag^2
    
    mov eax, edx
    imul eax, eax      ; threshold^2
    
    pop edx            ; Retrieve mag^2 into edx (swap roles)
    ; Now edx = mag^2, eax = threshold^2
    
    cmp edx, eax
    jge .amplify
    
.suppress:
    ; Reload values to modify
    mov eax, [esi + edi*8]
    mov ebx, [esi + edi*8 + 4]
    
    sar eax, 2
    sar ebx, 2
    jmp .store
    
.amplify:
    mov eax, [esi + edi*8]
    mov ebx, [esi + edi*8 + 4]
    
    mov edx, eax
    sar edx, 2
    add eax, edx    ; x * 1.25
    
    mov edx, ebx
    sar edx, 2
    add ebx, edx
    
.store:
    mov [esi + edi*8], eax
    mov [esi + edi*8 + 4], ebx
    
    inc edi
    jmp .loop
    
.end:
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret

; =============================================================================
; int32_t asm_calculate_entropy_fast(complex_fixed_t* buffer, uint32_t size)
; Arguments:
; [ebp + 8]  = buffer
; [ebp + 12] = size
; =============================================================================
asm_calculate_entropy_fast:
    push ebp
    mov ebp, esp
    push ebx
    push esi
    push edi
    
    mov esi, [ebp + 8]      ; buffer
    mov ecx, [ebp + 12]     ; size
    
    ; Step 1: Accumulate total power
    xor edi, edi            ; Total power (limited to 32 bit for simplicity)
    xor ebx, ebx            ; loop index
    
.acc_loop:
    cmp ebx, ecx
    jge .calc_entropy_start
    
    mov eax, [esi + ebx*8]
    imul eax, eax
    mov edx, [esi + ebx*8 + 4]
    imul edx, edx
    add eax, edx
    
    add edi, eax
    
    inc ebx
    jmp .acc_loop

.calc_entropy_start:
    test edi, edi
    jz .return_zero
    
    ; edi = Total Power
    xor ebx, ebx            ; index
    xor esi, esi            ; Entropy accumulator (we reuse esi, wait, esi was buffer. Need to reload or save buffer)
    
    ; Let's save buffer in [ebp-4] or just re-read args
    mov edx, [ebp + 8]      ; buffer ptr is needed, let's put it in a register. 
                            ; We are running out of registers.
                            ; Let's keep buffer in [ebp+8] and access with offset.
    
    xor esi, esi            ; Entropy accumulator
    
.p_loop:
    cmp ebx, ecx
    jge .finalize
    
    mov eax, [ebp + 8]      ; buffer base
    
    push ecx                ; save size
    push edi                ; save total power
    
    mov ecx, [eax + ebx*8]      ; real
    imul ecx, ecx
    mov edx, [eax + ebx*8 + 4]  ; imag
    imul edx, edx
    add ecx, edx                ; Power_i
    
    test ecx, ecx
    jz .next_iter_restore
    
    ; Approximating: term = (Power_i * (bsr(Power_i) - bsr(Total))) << 16 / Total
    
    bsr eax, ecx            ; log2(Power_i)
    mov edx, [esp]          ; Load Total (from stack top index 0: edi was pushed last... wait order: push ecx, push edi)
                            ; Stack: [edi (Total)] [ecx (Size)] ...
    
    bsr edx, edx            ; log2(Total)
    sub eax, edx            ; log diff (negative)
    
    imul eax, ecx           ; Power_i * log_diff
    shl eax, 16             ; Fixed point scaling
    
    ; Divide by Total
    ; edx is clobbered. Reload total.
    mov edx, [esp]          ; Total
    cdq                     ; Sign extend eax to edx:eax
    
    ; Wait, cdq overwrites edx (Total). We need to explicitly clear edx or setup logic.
    ; But idiv divides edx:eax by operand.
    ; We need edx:eax = value.
    ; So we need to move Total to a reg that is NOT edx or eax.
    
    mov edi, edx            ; Move Total to edi
    
    mov edx, eax            ; Save eax (value)
    sar edx, 31             ; Sign extend manually into edx ?? OR just use cdq on the value.
    
    ; Correct sequence:
    ; value in eax.
    cdq
    idiv edi                ; Divide edx:eax by edi (Total)
    
    sub esi, eax            ; Accumulate
    
.next_iter_restore:
    pop edi
    pop ecx
    
    inc ebx
    jmp .p_loop
    
.finalize:
    mov eax, esi
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret
    
.return_zero:
    xor eax, eax
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret

asm_fft_butterfly:
    ret
