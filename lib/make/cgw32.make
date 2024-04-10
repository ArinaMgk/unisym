# UNISYM for GCC-Win32 built-2024-04-11 01:11:19.742094

AASM = aasm
udir = E:/PROJ/SVGN/unisym

attr = -D_DEBUG -D_WinNT -O3 -D_Win32
cplpref=_ugc32_
cpppref=_uxxgc32_
dest_obj=../_obj/CGWin32
CC = E:/PROJ/CoStudioWin64/i686/bin/gcc.exe -m32
CX = E:/PROJ/CoStudioWin64/i686/bin/g++.exe -m32
AR = E:/PROJ/CoStudioWin64/i686/bin/ar.exe
aattr = -fwin32 -I$(udir)/inc/Kasha/n_
dest_abs = ../_bin/libw32d.a
.PHONY: all

all:
	-@E:/software/w64dev/bin/mkdir.exe -p ${dest_obj}
	-@E:/software/w64dev/bin/rm.exe -f ${dest_obj}/*
	cd ${dest_obj}/ && ${AASM} ${aattr} ../../unisym/lib/asm/x86/cpuid.asm -o cpuid_a.o
	cd ${dest_obj}/ && ${AASM} ${aattr} ../../unisym/lib/asm/x86/binary.asm -o binary_a.o
	$(CC) ./lib/c/auxiliary/toxxxer.make.c -o ../_tmp/toxxxer.make.exe && ../_tmp/toxxxer.make
	$(CC) -c ./lib/c/binary.c -o ${dest_obj}/${cplpref}binary.o $(attr)
	$(CC) -c ./lib/c/consio.c -o ${dest_obj}/${cplpref}consio.o $(attr)
	$(CC) -c ./lib/c/contable.c -o ${dest_obj}/${cplpref}contable.o $(attr)
	$(CC) -c ./lib/c/crc64.c -o ${dest_obj}/${cplpref}crc64.o $(attr)
	$(CC) -c ./lib/c/debug.c -o ${dest_obj}/${cplpref}debug.o $(attr)
	$(CC) -c ./lib/c/error.c -o ${dest_obj}/${cplpref}error.o $(attr)
	$(CC) -c ./lib/c/mcore.c -o ${dest_obj}/${cplpref}mcore.o $(attr)
	$(CC) -c ./lib/c/numar.c -o ${dest_obj}/${cplpref}numar.o $(attr)
	$(CC) -c ./lib/c/regar.c -o ${dest_obj}/${cplpref}regar.o $(attr)
	$(CC) -c ./lib/c/stack.c -o ${dest_obj}/${cplpref}stack.o $(attr)
	$(CC) -c ./lib/c/strpool.c -o ${dest_obj}/${cplpref}strpool.o $(attr)
	$(CC) -c ./lib/c/tenar.c -o ${dest_obj}/${cplpref}tenar.o $(attr)
	$(CC) -c ./lib/c/window.c -o ${dest_obj}/${cplpref}window.o $(attr)
	$(CC) -c ./lib/c/auxiliary/toxxxer.c -o ${dest_obj}/${cplpref}toxxxer.o $(attr)
	$(CC) -c ./lib/c/coear/CoeAcos.c -o ${dest_obj}/${cplpref}CoeAcos.o $(attr)
	$(CC) -c ./lib/c/coear/CoeAcosh.c -o ${dest_obj}/${cplpref}CoeAcosh.o $(attr)
	$(CC) -c ./lib/c/coear/CoeAdd.c -o ${dest_obj}/${cplpref}CoeAdd.o $(attr)
	$(CC) -c ./lib/c/coear/coear.c -o ${dest_obj}/${cplpref}coear.o $(attr)
	$(CC) -c ./lib/c/coear/CoeAsin.c -o ${dest_obj}/${cplpref}CoeAsin.o $(attr)
	$(CC) -c ./lib/c/coear/CoeAsinh.c -o ${dest_obj}/${cplpref}CoeAsinh.o $(attr)
	$(CC) -c ./lib/c/coear/CoeAtan.c -o ${dest_obj}/${cplpref}CoeAtan.o $(attr)
	$(CC) -c ./lib/c/coear/CoeAtanh.c -o ${dest_obj}/${cplpref}CoeAtanh.o $(attr)
	$(CC) -c ./lib/c/coear/CoeCmp.c -o ${dest_obj}/${cplpref}CoeCmp.o $(attr)
	$(CC) -c ./lib/c/coear/CoeCos.c -o ${dest_obj}/${cplpref}CoeCos.o $(attr)
	$(CC) -c ./lib/c/coear/CoeCosh.c -o ${dest_obj}/${cplpref}CoeCosh.o $(attr)
	$(CC) -c ./lib/c/coear/CoeCpy.c -o ${dest_obj}/${cplpref}CoeCpy.o $(attr)
	$(CC) -c ./lib/c/coear/CoeCtz.c -o ${dest_obj}/${cplpref}CoeCtz.o $(attr)
	$(CC) -c ./lib/c/coear/CoeDel.c -o ${dest_obj}/${cplpref}CoeDel.o $(attr)
	$(CC) -c ./lib/c/coear/CoeDig.c -o ${dest_obj}/${cplpref}CoeDig.o $(attr)
	$(CC) -c ./lib/c/coear/CoeDiv.c -o ${dest_obj}/${cplpref}CoeDiv.o $(attr)
	$(CC) -c ./lib/c/coear/CoeDivrAlign.c -o ${dest_obj}/${cplpref}CoeDivrAlign.o $(attr)
	$(CC) -c ./lib/c/coear/CoeDivrUnit.c -o ${dest_obj}/${cplpref}CoeDivrUnit.o $(attr)
	$(CC) -c ./lib/c/coear/CoeE.c -o ${dest_obj}/${cplpref}CoeE.o $(attr)
	$(CC) -c ./lib/c/coear/CoeExp.c -o ${dest_obj}/${cplpref}CoeExp.o $(attr)
	$(CC) -c ./lib/c/coear/CoeExpoAlign.c -o ${dest_obj}/${cplpref}CoeExpoAlign.o $(attr)
	$(CC) -c ./lib/c/coear/CoeFac.c -o ${dest_obj}/${cplpref}CoeFac.o $(attr)
	$(CC) -c ./lib/c/coear/CoeFromDouble.c -o ${dest_obj}/${cplpref}CoeFromDouble.o $(attr)
	$(CC) -c ./lib/c/coear/CoeFromInteger.c -o ${dest_obj}/${cplpref}CoeFromInteger.o $(attr)
	$(CC) -c ./lib/c/coear/CoeFromLocale.c -o ${dest_obj}/${cplpref}CoeFromLocale.o $(attr)
	$(CC) -c ./lib/c/coear/CoeHypot.c -o ${dest_obj}/${cplpref}CoeHypot.o $(attr)
	$(CC) -c ./lib/c/coear/CoeInit.c -o ${dest_obj}/${cplpref}CoeInit.o $(attr)
	$(CC) -c ./lib/c/coear/CoeInt.c -o ${dest_obj}/${cplpref}CoeInt.o $(attr)
	$(CC) -c ./lib/c/coear/CoeIsINF.c -o ${dest_obj}/${cplpref}CoeIsINF.o $(attr)
	$(CC) -c ./lib/c/coear/CoeIsNAN.c -o ${dest_obj}/${cplpref}CoeIsNAN.o $(attr)
	$(CC) -c ./lib/c/coear/CoeLog.c -o ${dest_obj}/${cplpref}CoeLog.o $(attr)
	$(CC) -c ./lib/c/coear/CoeMul.c -o ${dest_obj}/${cplpref}CoeMul.o $(attr)
	$(CC) -c ./lib/c/coear/CoeNeg.c -o ${dest_obj}/${cplpref}CoeNeg.o $(attr)
	$(CC) -c ./lib/c/coear/CoeNew.c -o ${dest_obj}/${cplpref}CoeNew.o $(attr)
	$(CC) -c ./lib/c/coear/CoePi.c -o ${dest_obj}/${cplpref}CoePi.o $(attr)
	$(CC) -c ./lib/c/coear/CoePow.c -o ${dest_obj}/${cplpref}CoePow.o $(attr)
	$(CC) -c ./lib/c/coear/CoeRst.c -o ${dest_obj}/${cplpref}CoeRst.o $(attr)
	$(CC) -c ./lib/c/coear/CoeSgn.c -o ${dest_obj}/${cplpref}CoeSgn.o $(attr)
	$(CC) -c ./lib/c/coear/CoeSin.c -o ${dest_obj}/${cplpref}CoeSin.o $(attr)
	$(CC) -c ./lib/c/coear/CoeSinh.c -o ${dest_obj}/${cplpref}CoeSinh.o $(attr)
	$(CC) -c ./lib/c/coear/CoeSqrt.c -o ${dest_obj}/${cplpref}CoeSqrt.o $(attr)
	$(CC) -c ./lib/c/coear/CoeSquare.c -o ${dest_obj}/${cplpref}CoeSquare.o $(attr)
	$(CC) -c ./lib/c/coear/CoeSub.c -o ${dest_obj}/${cplpref}CoeSub.o $(attr)
	$(CC) -c ./lib/c/coear/CoeTan.c -o ${dest_obj}/${cplpref}CoeTan.o $(attr)
	$(CC) -c ./lib/c/coear/CoeTanh.c -o ${dest_obj}/${cplpref}CoeTanh.o $(attr)
	$(CC) -c ./lib/c/coear/CoeTaylor.c -o ${dest_obj}/${cplpref}CoeTaylor.o $(attr)
	$(CC) -c ./lib/c/coear/CoeToDouble.c -o ${dest_obj}/${cplpref}CoeToDouble.o $(attr)
	$(CC) -c ./lib/c/coear/CoeToLocale.c -o ${dest_obj}/${cplpref}CoeToLocale.o $(attr)
	$(CC) -c ./lib/c/datime/fromherp.c -o ${dest_obj}/${cplpref}fromherp.o $(attr)
	$(CC) -c ./lib/c/datime/POSIXGetSeconds.c -o ${dest_obj}/${cplpref}POSIXGetSeconds.o $(attr)
	$(CC) -c ./lib/c/datime/weekday.c -o ${dest_obj}/${cplpref}weekday.o $(attr)
	$(CC) -c ./lib/c/dnode/dnode.c -o ${dest_obj}/${cplpref}dnode.o $(attr)
	$(CC) -c ./lib/c/dnode/DnodeAppend.c -o ${dest_obj}/${cplpref}DnodeAppend.o $(attr)
	$(CC) -c ./lib/c/dnode/DnodeBind.c -o ${dest_obj}/${cplpref}DnodeBind.o $(attr)
	$(CC) -c ./lib/c/dnode/DnodeCount.c -o ${dest_obj}/${cplpref}DnodeCount.o $(attr)
	$(CC) -c ./lib/c/dnode/DnodeIndex.c -o ${dest_obj}/${cplpref}DnodeIndex.o $(attr)
	$(CC) -c ./lib/c/dnode/DnodeInsert.c -o ${dest_obj}/${cplpref}DnodeInsert.o $(attr)
	$(CC) -c ./lib/c/dnode/DnodeRelease.c -o ${dest_obj}/${cplpref}DnodeRelease.o $(attr)
	$(CC) -c ./lib/c/dnode/DnodeRemove.c -o ${dest_obj}/${cplpref}DnodeRemove.o $(attr)
	$(CC) -c ./lib/c/dnode/DnodeRewind.c -o ${dest_obj}/${cplpref}DnodeRewind.o $(attr)
	$(CC) -c ./lib/c/dnode/DnodeSort.c -o ${dest_obj}/${cplpref}DnodeSort.o $(attr)
	$(CC) -c ./lib/c/inode/InodeDelete.c -o ${dest_obj}/${cplpref}InodeDelete.o $(attr)
	$(CC) -c ./lib/c/inode/InodeLocate.c -o ${dest_obj}/${cplpref}InodeLocate.o $(attr)
	$(CC) -c ./lib/c/inode/InodesRelease.c -o ${dest_obj}/${cplpref}InodesRelease.o $(attr)
	$(CC) -c ./lib/c/inode/InodeUpdate.c -o ${dest_obj}/${cplpref}InodeUpdate.o $(attr)
	$(CC) -c ./lib/c/math/factorial.c -o ${dest_obj}/${cplpref}factorial.o $(attr)
	$(CC) -c ./lib/c/math/taylor.c -o ${dest_obj}/${cplpref}taylor.o $(attr)
	$(CC) -c ./lib/c/nnode/nnode.c -o ${dest_obj}/${cplpref}nnode.o $(attr)
	$(CC) -c ./lib/c/nnode/NnodeBlock.c -o ${dest_obj}/${cplpref}NnodeBlock.o $(attr)
	$(CC) -c ./lib/c/nnode/NnodeInsert.c -o ${dest_obj}/${cplpref}NnodeInsert.o $(attr)
	$(CC) -c ./lib/c/nnode/NnodeRelease.c -o ${dest_obj}/${cplpref}NnodeRelease.o $(attr)
	$(CC) -c ./lib/c/nnode/NnodesRelease.c -o ${dest_obj}/${cplpref}NnodesRelease.o $(attr)
	$(CC) -c ./lib/c/node/node.c -o ${dest_obj}/${cplpref}node.o $(attr)
	$(CC) -c ./lib/c/node/NodeAppend.c -o ${dest_obj}/${cplpref}NodeAppend.o $(attr)
	$(CC) -c ./lib/c/node/NodeCount.c -o ${dest_obj}/${cplpref}NodeCount.o $(attr)
	$(CC) -c ./lib/c/node/NodeIndex.c -o ${dest_obj}/${cplpref}NodeIndex.o $(attr)
	$(CC) -c ./lib/c/node/NodeInsert.c -o ${dest_obj}/${cplpref}NodeInsert.o $(attr)
	$(CC) -c ./lib/c/node/NodeRelease.c -o ${dest_obj}/${cplpref}NodeRelease.o $(attr)
	$(CC) -c ./lib/c/node/NodeRemove.c -o ${dest_obj}/${cplpref}NodeRemove.o $(attr)
	$(CC) -c ./lib/c/node/NodeSort.c -o ${dest_obj}/${cplpref}NodeSort.o $(attr)
	$(CC) -c ./lib/c/tnode/StrTokenAll.c -o ${dest_obj}/${cplpref}StrTokenAll.o $(attr)
	$(CC) -c ./lib/c/tnode/StrTokenAppend.c -o ${dest_obj}/${cplpref}StrTokenAppend.o $(attr)
	$(CC) -c ./lib/c/tnode/StrTokenClearAll.c -o ${dest_obj}/${cplpref}StrTokenClearAll.o $(attr)
	$(CC) -c ./lib/c/tnode/StrTokenThrow.c -o ${dest_obj}/${cplpref}StrTokenThrow.o $(attr)
	$(CC) -c ./lib/c/tnode/tnode.c -o ${dest_obj}/${cplpref}tnode.o $(attr)
	$(CC) -c ./lib/c/tnode/TnodesReleases.c -o ${dest_obj}/${cplpref}TnodesReleases.o $(attr)
	$(CC) -c ./lib/c/ustring/astring/atobcd.c -o ${dest_obj}/${cplpref}atobcd.o $(attr)
	$(CC) -c ./lib/c/ustring/astring/bcdtoa.c -o ${dest_obj}/${cplpref}bcdtoa.o $(attr)
	$(CC) -c ./lib/c/ustring/astring/instoa.c -o ${dest_obj}/${cplpref}instoa.o $(attr)
	$(CC) -c ./lib/c/ustring/astring/MemHeap.c -o ${dest_obj}/${cplpref}MemHeap.o $(attr)
	$(CC) -c ./lib/c/ustring/astring/StrHeap.c -o ${dest_obj}/${cplpref}StrHeap.o $(attr)
	$(CC) -c ./lib/c/ustring/astring/StrHeapAppend.c -o ${dest_obj}/${cplpref}StrHeapAppend.o $(attr)
	$(CC) -c ./lib/c/ustring/astring/StrHeapAppendChars.c -o ${dest_obj}/${cplpref}StrHeapAppendChars.o $(attr)
	$(CC) -c ./lib/c/ustring/astring/StrHeapAppendN.c -o ${dest_obj}/${cplpref}StrHeapAppendN.o $(attr)
	$(CC) -c ./lib/c/ustring/astring/StrHeapFromChar.c -o ${dest_obj}/${cplpref}StrHeapFromChar.o $(attr)
	$(CC) -c ./lib/c/ustring/astring/StrHeapInsertThrow.c -o ${dest_obj}/${cplpref}StrHeapInsertThrow.o $(attr)
	$(CC) -c ./lib/c/ustring/astring/StrHeapN.c -o ${dest_obj}/${cplpref}StrHeapN.o $(attr)
	$(CC) -c ./lib/c/ustring/astring/StrReplace.c -o ${dest_obj}/${cplpref}StrReplace.o $(attr)
	$(CC) -c ./lib/c/ustring/chrar/ChrAdd.c -o ${dest_obj}/${cplpref}ChrAdd.o $(attr)
	$(CC) -c ./lib/c/ustring/chrar/ChrArrange.c -o ${dest_obj}/${cplpref}ChrArrange.o $(attr)
	$(CC) -c ./lib/c/ustring/chrar/ChrCmp.c -o ${dest_obj}/${cplpref}ChrCmp.o $(attr)
	$(CC) -c ./lib/c/ustring/chrar/ChrCombinate.c -o ${dest_obj}/${cplpref}ChrCombinate.o $(attr)
	$(CC) -c ./lib/c/ustring/chrar/ChrComDiv.c -o ${dest_obj}/${cplpref}ChrComDiv.o $(attr)
	$(CC) -c ./lib/c/ustring/chrar/ChrComMul.c -o ${dest_obj}/${cplpref}ChrComMul.o $(attr)
	$(CC) -c ./lib/c/ustring/chrar/ChrCpz.c -o ${dest_obj}/${cplpref}ChrCpz.o $(attr)
	$(CC) -c ./lib/c/ustring/chrar/ChrCtz.c -o ${dest_obj}/${cplpref}ChrCtz.o $(attr)
	$(CC) -c ./lib/c/ustring/chrar/ChrDecToHex.c -o ${dest_obj}/${cplpref}ChrDecToHex.o $(attr)
	$(CC) -c ./lib/c/ustring/chrar/ChrDecToHexFloat.c -o ${dest_obj}/${cplpref}ChrDecToHexFloat.o $(attr)
	$(CC) -c ./lib/c/ustring/chrar/ChrDiv.c -o ${dest_obj}/${cplpref}ChrDiv.o $(attr)
	$(CC) -c ./lib/c/ustring/chrar/ChrFactorial.c -o ${dest_obj}/${cplpref}ChrFactorial.o $(attr)
	$(CC) -c ./lib/c/ustring/chrar/ChrHexToDec.c -o ${dest_obj}/${cplpref}ChrHexToDec.o $(attr)
	$(CC) -c ./lib/c/ustring/chrar/ChrHexToDecFloat.c -o ${dest_obj}/${cplpref}ChrHexToDecFloat.o $(attr)
	$(CC) -c ./lib/c/ustring/chrar/ChrInsPow.c -o ${dest_obj}/${cplpref}ChrInsPow.o $(attr)
	$(CC) -c ./lib/c/ustring/chrar/ChrMul.c -o ${dest_obj}/${cplpref}ChrMul.o $(attr)
	$(CC) -c ./lib/c/ustring/chrar/ChrSub.c -o ${dest_obj}/${cplpref}ChrSub.o $(attr)
	$(CC) -c ./lib/c/ustring/chrar/DigDec.c -o ${dest_obj}/${cplpref}DigDec.o $(attr)
	$(CC) -c ./lib/c/ustring/chrar/DigInc.c -o ${dest_obj}/${cplpref}DigInc.o $(attr)
	$(CC) -c ./lib/c/ustring/gstring/atoins.c -o ${dest_obj}/${cplpref}atoins.o $(attr)
	$(CC) -c ./lib/c/ustring/gstring/instob.c -o ${dest_obj}/${cplpref}instob.o $(attr)
	$(CC) -c ./lib/c/ustring/gstring/MemCompareRight.c -o ${dest_obj}/${cplpref}MemCompareRight.o $(attr)
	$(CC) -c ./lib/c/ustring/gstring/MemSet.c -o ${dest_obj}/${cplpref}MemSet.o $(attr)
	$(CC) -c ./lib/c/ustring/gstring/StrDeprefixSpaces.c -o ${dest_obj}/${cplpref}StrDeprefixSpaces.o $(attr)
	$(CC) -c ./lib/c/ustring/gstring/StrDesuffixSpaces.c -o ${dest_obj}/${cplpref}StrDesuffixSpaces.o $(attr)
	$(CC) -c ./lib/c/ustring/gstring/StrFilter.c -o ${dest_obj}/${cplpref}StrFilter.o $(attr)
	$(CC) -c ./lib/c/ustring/gstring/StrFilterOut.c -o ${dest_obj}/${cplpref}StrFilterOut.o $(attr)
	$(CC) -c ./lib/c/ustring/gstring/StrFilterOutString.c -o ${dest_obj}/${cplpref}StrFilterOutString.o $(attr)
	$(CC) -c ./lib/c/ustring/gstring/StrFilterString.c -o ${dest_obj}/${cplpref}StrFilterString.o $(attr)
	$(CC) -c ./lib/c/ustring/gstring/StrShiftLeft4.c -o ${dest_obj}/${cplpref}StrShiftLeft4.o $(attr)
	$(CC) -c ./lib/c/ustring/gstring/StrShiftLeft8n.c -o ${dest_obj}/${cplpref}StrShiftLeft8n.o $(attr)
	$(CC) -c ./lib/c/ustring/gstring/StrShiftRight4.c -o ${dest_obj}/${cplpref}StrShiftRight4.o $(attr)
	$(CC) -c ./lib/c/ustring/gstring/StrShiftRight8n.c -o ${dest_obj}/${cplpref}StrShiftRight8n.o $(attr)
	$(CC) -c ./lib/c/ustring/gstring/StrSortBubble.c -o ${dest_obj}/${cplpref}StrSortBubble.o $(attr)
	$(CC) -c ./lib/c/ustring/gstring/_size_decimal_get.c -o ${dest_obj}/${cplpref}_size_decimal_get.o $(attr)
	$(CX) -c ./lib/cpp/coe.cpp -o ${dest_obj}/${cpppref}coe.o $(attr)
	$(CX) -c ./lib/cpp/consio.cpp -o ${dest_obj}/${cpppref}consio.o $(attr)
	$(CX) -c ./lib/cpp/debug.cpp -o ${dest_obj}/${cpppref}debug.o $(attr)
	$(CX) -c ./lib/cpp/floating.cpp -o ${dest_obj}/${cpppref}floating.o $(attr)
	$(CX) -c ./lib/cpp/numberx.cpp -o ${dest_obj}/${cpppref}numberx.o $(attr)
	$(CX) -c ./lib/cpp/stream.cpp -o ${dest_obj}/${cpppref}stream.o $(attr)
	$(CX) -c ./lib/cpp/string.cpp -o ${dest_obj}/${cpppref}string.o $(attr)
	$(CX) -c ./lib/cpp/strpool.cpp -o ${dest_obj}/${cpppref}strpool.o $(attr)
	$(CX) -c ./lib/cpp/Device/GPIO.cpp -o ${dest_obj}/${cpppref}GPIO.o $(attr)
	$(CX) -c ./lib/cpp/nodes/dnode.cpp -o ${dest_obj}/${cpppref}dnode.o $(attr)
	$(CX) -c ./lib/cpp/nodes/inode.cpp -o ${dest_obj}/${cpppref}inode.o $(attr)
	$(CX) -c ./lib/cpp/nodes/nnode.cpp -o ${dest_obj}/${cpppref}nnode.o $(attr)
	$(CX) -c ./lib/cpp/nodes/node.cpp -o ${dest_obj}/${cpppref}node.o $(attr)
	$(CX) -c ./lib/cpp/nodes/tnode.cpp -o ${dest_obj}/${cpppref}tnode.o $(attr)
	$(CX) -c ./lib/cpp/nodes/nnode/NnodeParse.cpp -o ${dest_obj}/${cpppref}NnodeParse.o $(attr)
	$(CX) -c ./lib/cpp/nodes/nnode/NnodeParse/NnodeParseOperator.cpp -o ${dest_obj}/${cpppref}NnodeParseOperator.o $(attr)
	$(CX) -c ./lib/cpp/nodes/tnode/TokenParse.cpp -o ${dest_obj}/${cpppref}TokenParse.o $(attr)
	${AR} -rcs ${dest_abs} ${dest_obj}/*
