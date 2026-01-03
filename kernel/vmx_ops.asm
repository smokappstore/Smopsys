[BITS 32]

global vmx_vmxon
global vmx_vmxoff
global vmx_vmptrld
global vmx_vmlaunch
global vmx_vmresume
global vmx_vmread
global vmx_vmwrite

section .text

; void vmx_vmxon(uint32_t phys_addr_low, uint32_t phys_addr_high)
; 64-bit address is split, but in 32-bit mode we typically use PAE or just low part if < 4GB.
; The argument here assumes we pass a 64-bit pointer or address.
; For 32-bit flat mode, we'll assume the address is on stack.
; VMXON takes a 64-bit memory operand.
vmx_vmxon:
    push ebp
    mov ebp, esp
    ; VMXON requires a pointer to a 64-bit physical address.
    ; Arguments are at ebp+8 (low) and ebp+12 (high)
    ; We need to construct this in memory.
    sub esp, 8
    mov eax, [ebp+8]
    mov [esp], eax
    mov eax, [ebp+12]
    mov [esp+4], eax
    
    vmxon [esp]
    jbe .error
    
    add esp, 8
    pop ebp
    ret
.error:
    ; Handle error (maybe set a flag or panic)
    ; For now just return
    add esp, 8
    pop ebp
    ret

vmx_vmxoff:
    vmxoff
    ret

; void vmx_vmptrld(uint32_t phys_addr_low, uint32_t phys_addr_high)
vmx_vmptrld:
    push ebp
    mov ebp, esp
    sub esp, 8
    mov eax, [ebp+8]
    mov [esp], eax
    mov eax, [ebp+12]
    mov [esp+4], eax
    
    vmptrld [esp]
    
    add esp, 8
    pop ebp
    ret

vmx_vmlaunch:
    vmlaunch
    ; If we get here, it failed
    ret

vmx_vmresume:
    vmresume
    ; If we get here, it failed
    ret

; uint32_t vmx_vmread(uint32_t field)
; Returns value in EAX
vmx_vmread:
    push ebp
    mov ebp, esp
    push ecx
    push edx
    
    mov eax, [ebp+8] ; field
    vmread ecx, eax  ; Not quite, VMREAD reads FROM field (src op) TO dest (reg/mem)
                     ; Syntax: vmread r/m32, r32  => reads from VMCS field specified by source operand?
                     ; No, "VMREAD r/m64, r64" -> Reads a specified VMCS field (in r64) to dest?
                     ; Actually: VMREAD dest, field_encoding?
                     ; Intel manual: "VMREAD r/m64, r64" is incorrect. 
                     ; Opcode is "0F 78 /r" implies VMREAD r32, r/m32 (where r/m32 is source field encoding?)
                     ; Wait, VMREAD reads FROM the VMCS field specified by the source operand (register)
                     ; and stores it into the destination operand.
                     ; So: vmread dest, field_encoding_reg
    
    mov ecx, eax     ; ecx = field encoding
    vmread eax, ecx  ; read field (ecx) into eax
    
    pop edx
    pop ecx
    pop ebp
    ret

; void vmx_vmwrite(uint32_t field, uint32_t value)
vmx_vmwrite:
    push ebp
    mov ebp, esp
    
    mov eax, [ebp+8]  ; field
    mov ecx, [ebp+12] ; value
    
    ; VMWRITE field_encoding_reg, value_reg
    vmwrite eax, ecx
    
    pop ebp
    ret

; =========================================================
; VM EXIT HANDLER
; =========================================================
[EXTERN vm_exit_handler]
global vm_exit_handler_stub

vm_exit_handler_stub:
    ; Hardware saves some state in VMCS Guest State fields
    ; But we are back in Host State here (defined by VMCS Host State fields)
    ; We are responsible for saving GPRs if we want to return to same state
    
    pushad              ; Save all 32-bit registers (eax, ecx, edx, ebx, esp, ebp, esi, edi)
    
    call vm_exit_handler
    
    popad               ; Restore registers
    
    vmresume            ; Resume VM
    ; If VMRESUME fails
    jmp .exit_failure

.exit_failure:
    ; We should probably panic or print error code
    ; For now, just spin
    cli
    hlt
    jmp .exit_failure
