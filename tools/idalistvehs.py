
import idc
import idaapi
import idautils

def clean_name(addr):
    try:
        raw_name = idc.get_func_name(addr)
        name = idaapi.demangle_name(raw_name, idaapi.get_inf_structure().demnames)
        return name.split('(')[0].split(' ')[-1]
    except:
        return GetFunctionName(addr)

num = 0
ptr = 0x952828  # Vehs array base
size = 0x01A034 # Vehs array size
end = ptr + size

for seg in idautils.Segments():
    for head in idautils.Heads(seg, idc.get_segm_end(seg)):
        if idaapi.is_code(idaapi.get_full_flags(head)):
            mne = idc.print_insn_mnem(head)
            for op in range(idaapi.UA_MAXOP):
                optype = idc.get_operand_type(head, op)
                is_reg = op == 0 and optype == idc.o_reg and mne != 'cmp'
                is_imm = op == 0 and optype == idc.o_imm and mne == 'push'
                is_mov = op == 1 and optype == idc.o_imm and mne == 'mov'
                if is_reg or is_imm or is_mov or optype in (idc.o_mem, idc.o_displ):
                    value = idc.get_operand_value(head, 1 if is_reg else op)
                    if value >= ptr and value < end:
                        name = clean_name(head)
                        insn = idaapi.insn_t()
                        ilen = idaapi.decode_insn(insn, head)
                        offset = []
                        for iop in insn.ops:
                            if not is_mov and iop.type in (idc.o_mem, idc.o_displ):
                                offset.append(iop.offb)
                            elif (is_reg or is_imm or is_mov) and iop.type == idc.o_imm:
                                offset.append(iop.offb)
                        assert(ilen > 0)
                        assert(len(offset) == 1)
                        assert(offset[0] > 0 and offset[0] <= ilen - 4)
                        addr = head + offset[0]
                        disp = Dword(addr) - ptr
                        print('{0x%X, 0x%02X}, // %s' % (addr, disp, name))
                        num += 1
                        break # Skip repeated lines

print('NUM %d' % (num))

