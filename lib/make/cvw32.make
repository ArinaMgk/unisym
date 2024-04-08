# UNISYM for MSVC-Win32 built-2024-04-08 22:25:01.566859

AASM = aasm
udir = E:/PROJ/SVGN/unisym

attr = /D_DEBUG /D_WinNT /D_Win32
cplpref=_uvc32_
cpppref=_uxxvc32_
dest_obj=../_obj/CVWin32
TOOLDIR = E:/software/VS22/VC/Tools/MSVC/14.39.33519
CC = ${TOOLDIR}/bin/Hostx86/x86/cl.exe
CX = ${CC}
AR = ${TOOLDIR}/bin/Hostx86/x86/lib.exe
# contain x86 - x64 can run this
aattr = -fwin32 -I../../unisym/inc/Kasha/n_
dest_abs = ../_bin/libw32d.lib
VLIB_64=/LIBPATH:"${TOOLDIR}/lib/x86/" /LIBPATH:"${TOOLDIR}/lib/onecore/x86" /LIBPATH:"C:/Program Files (x86)/Windows Kits/10/Lib/10.0.19041.0/um/x86" /LIBPATH:"C:/Program Files (x86)/Windows Kits/10/Lib/10.0.19041.0/ucrt/x86"
VI_SYS="C:/Program Files (x86)/Windows Kits/10/Include/10.0.19041.0/ucrt/"
VI_64=${TOOLDIR}/include/ /I${VI_SYS} /I"C:/Program Files (x86)/Windows Kits/10/Include/10.0.19041.0/um/" /I"C:/Program Files (x86)/Windows Kits/10/Include/10.0.19041.0/shared/"
.PHONY: all

all:
	-@E:/software/w64dev/bin/mkdir.exe -p ${dest_obj}
	-@E:/software/w64dev/bin/rm.exe -f ${dest_obj}/*
	cd ${dest_obj}/ && ${AASM} ${aattr} ../../unisym/lib/asm/x86/cpuid.asm -o cpuid_a.o
	cd ${dest_obj}/ && ${AASM} ${aattr} ../../unisym/lib/asm/x86/binary.asm -o binary_a.o
	$(CC) ./lib/c/auxiliary/toxxxer.make.c /Fe: ../_tmp/toxxxer.make.exe /Fo: ../_tmp/toxxxer.make.obj /I${VI_64} ${attr} /link${VLIB_64} && ../_tmp/toxxxer.make&& echo toxxxer.make
	$(CC) /c ./lib/c/binary.c /Fo:${dest_obj}/${cplpref}binary.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/consio.c /Fo:${dest_obj}/${cplpref}consio.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/contable.c /Fo:${dest_obj}/${cplpref}contable.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/crc64.c /Fo:${dest_obj}/${cplpref}crc64.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/debug.c /Fo:${dest_obj}/${cplpref}debug.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/error.c /Fo:${dest_obj}/${cplpref}error.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/mcore.c /Fo:${dest_obj}/${cplpref}mcore.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/numar.c /Fo:${dest_obj}/${cplpref}numar.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/regar.c /Fo:${dest_obj}/${cplpref}regar.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/stack.c /Fo:${dest_obj}/${cplpref}stack.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/strpool.c /Fo:${dest_obj}/${cplpref}strpool.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/tenar.c /Fo:${dest_obj}/${cplpref}tenar.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/window.c /Fo:${dest_obj}/${cplpref}window.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/auxiliary/toxxxer.c /Fo:${dest_obj}/${cplpref}toxxxer.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/coear/CoeAcos.c /Fo:${dest_obj}/${cplpref}CoeAcos.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/coear/CoeAcosh.c /Fo:${dest_obj}/${cplpref}CoeAcosh.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/coear/CoeAdd.c /Fo:${dest_obj}/${cplpref}CoeAdd.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/coear/coear.c /Fo:${dest_obj}/${cplpref}coear.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/coear/CoeAsin.c /Fo:${dest_obj}/${cplpref}CoeAsin.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/coear/CoeAsinh.c /Fo:${dest_obj}/${cplpref}CoeAsinh.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/coear/CoeAtan.c /Fo:${dest_obj}/${cplpref}CoeAtan.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/coear/CoeAtanh.c /Fo:${dest_obj}/${cplpref}CoeAtanh.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/coear/CoeCmp.c /Fo:${dest_obj}/${cplpref}CoeCmp.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/coear/CoeCos.c /Fo:${dest_obj}/${cplpref}CoeCos.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/coear/CoeCosh.c /Fo:${dest_obj}/${cplpref}CoeCosh.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/coear/CoeCpy.c /Fo:${dest_obj}/${cplpref}CoeCpy.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/coear/CoeCtz.c /Fo:${dest_obj}/${cplpref}CoeCtz.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/coear/CoeDel.c /Fo:${dest_obj}/${cplpref}CoeDel.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/coear/CoeDig.c /Fo:${dest_obj}/${cplpref}CoeDig.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/coear/CoeDiv.c /Fo:${dest_obj}/${cplpref}CoeDiv.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/coear/CoeDivrAlign.c /Fo:${dest_obj}/${cplpref}CoeDivrAlign.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/coear/CoeDivrUnit.c /Fo:${dest_obj}/${cplpref}CoeDivrUnit.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/coear/CoeE.c /Fo:${dest_obj}/${cplpref}CoeE.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/coear/CoeExp.c /Fo:${dest_obj}/${cplpref}CoeExp.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/coear/CoeExpoAlign.c /Fo:${dest_obj}/${cplpref}CoeExpoAlign.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/coear/CoeFac.c /Fo:${dest_obj}/${cplpref}CoeFac.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/coear/CoeFromDouble.c /Fo:${dest_obj}/${cplpref}CoeFromDouble.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/coear/CoeFromInteger.c /Fo:${dest_obj}/${cplpref}CoeFromInteger.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/coear/CoeFromLocale.c /Fo:${dest_obj}/${cplpref}CoeFromLocale.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/coear/CoeHypot.c /Fo:${dest_obj}/${cplpref}CoeHypot.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/coear/CoeInit.c /Fo:${dest_obj}/${cplpref}CoeInit.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/coear/CoeInt.c /Fo:${dest_obj}/${cplpref}CoeInt.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/coear/CoeIsINF.c /Fo:${dest_obj}/${cplpref}CoeIsINF.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/coear/CoeIsNAN.c /Fo:${dest_obj}/${cplpref}CoeIsNAN.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/coear/CoeLog.c /Fo:${dest_obj}/${cplpref}CoeLog.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/coear/CoeMul.c /Fo:${dest_obj}/${cplpref}CoeMul.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/coear/CoeNeg.c /Fo:${dest_obj}/${cplpref}CoeNeg.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/coear/CoeNew.c /Fo:${dest_obj}/${cplpref}CoeNew.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/coear/CoePi.c /Fo:${dest_obj}/${cplpref}CoePi.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/coear/CoePow.c /Fo:${dest_obj}/${cplpref}CoePow.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/coear/CoeRst.c /Fo:${dest_obj}/${cplpref}CoeRst.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/coear/CoeSgn.c /Fo:${dest_obj}/${cplpref}CoeSgn.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/coear/CoeSin.c /Fo:${dest_obj}/${cplpref}CoeSin.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/coear/CoeSinh.c /Fo:${dest_obj}/${cplpref}CoeSinh.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/coear/CoeSqrt.c /Fo:${dest_obj}/${cplpref}CoeSqrt.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/coear/CoeSquare.c /Fo:${dest_obj}/${cplpref}CoeSquare.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/coear/CoeSub.c /Fo:${dest_obj}/${cplpref}CoeSub.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/coear/CoeTan.c /Fo:${dest_obj}/${cplpref}CoeTan.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/coear/CoeTanh.c /Fo:${dest_obj}/${cplpref}CoeTanh.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/coear/CoeTaylor.c /Fo:${dest_obj}/${cplpref}CoeTaylor.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/coear/CoeToDouble.c /Fo:${dest_obj}/${cplpref}CoeToDouble.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/coear/CoeToLocale.c /Fo:${dest_obj}/${cplpref}CoeToLocale.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/datime/fromherp.c /Fo:${dest_obj}/${cplpref}fromherp.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/datime/POSIXGetSeconds.c /Fo:${dest_obj}/${cplpref}POSIXGetSeconds.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/datime/weekday.c /Fo:${dest_obj}/${cplpref}weekday.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/dnode/dnode.c /Fo:${dest_obj}/${cplpref}dnode.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/dnode/DnodeAppend.c /Fo:${dest_obj}/${cplpref}DnodeAppend.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/dnode/DnodeBind.c /Fo:${dest_obj}/${cplpref}DnodeBind.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/dnode/DnodeCount.c /Fo:${dest_obj}/${cplpref}DnodeCount.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/dnode/DnodeIndex.c /Fo:${dest_obj}/${cplpref}DnodeIndex.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/dnode/DnodeInsert.c /Fo:${dest_obj}/${cplpref}DnodeInsert.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/dnode/DnodeRelease.c /Fo:${dest_obj}/${cplpref}DnodeRelease.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/dnode/DnodeRemove.c /Fo:${dest_obj}/${cplpref}DnodeRemove.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/dnode/DnodeRewind.c /Fo:${dest_obj}/${cplpref}DnodeRewind.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/dnode/DnodeSort.c /Fo:${dest_obj}/${cplpref}DnodeSort.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/inode/InodeDelete.c /Fo:${dest_obj}/${cplpref}InodeDelete.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/inode/InodeLocate.c /Fo:${dest_obj}/${cplpref}InodeLocate.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/inode/InodesRelease.c /Fo:${dest_obj}/${cplpref}InodesRelease.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/inode/InodeUpdate.c /Fo:${dest_obj}/${cplpref}InodeUpdate.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/nnode/nnode.c /Fo:${dest_obj}/${cplpref}nnode.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/nnode/NnodeBlock.c /Fo:${dest_obj}/${cplpref}NnodeBlock.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/nnode/NnodeInsert.c /Fo:${dest_obj}/${cplpref}NnodeInsert.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/nnode/NnodeRelease.c /Fo:${dest_obj}/${cplpref}NnodeRelease.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/nnode/NnodesRelease.c /Fo:${dest_obj}/${cplpref}NnodesRelease.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/node/node.c /Fo:${dest_obj}/${cplpref}node.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/node/NodeAppend.c /Fo:${dest_obj}/${cplpref}NodeAppend.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/node/NodeCount.c /Fo:${dest_obj}/${cplpref}NodeCount.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/node/NodeIndex.c /Fo:${dest_obj}/${cplpref}NodeIndex.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/node/NodeInsert.c /Fo:${dest_obj}/${cplpref}NodeInsert.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/node/NodeRelease.c /Fo:${dest_obj}/${cplpref}NodeRelease.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/node/NodeRemove.c /Fo:${dest_obj}/${cplpref}NodeRemove.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/node/NodeSort.c /Fo:${dest_obj}/${cplpref}NodeSort.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/tnode/StrTokenAll.c /Fo:${dest_obj}/${cplpref}StrTokenAll.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/tnode/StrTokenAppend.c /Fo:${dest_obj}/${cplpref}StrTokenAppend.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/tnode/StrTokenClearAll.c /Fo:${dest_obj}/${cplpref}StrTokenClearAll.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/tnode/StrTokenThrow.c /Fo:${dest_obj}/${cplpref}StrTokenThrow.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/tnode/tnode.c /Fo:${dest_obj}/${cplpref}tnode.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/tnode/TnodesReleases.c /Fo:${dest_obj}/${cplpref}TnodesReleases.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/ustring/astring/atobcd.c /Fo:${dest_obj}/${cplpref}atobcd.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/ustring/astring/bcdtoa.c /Fo:${dest_obj}/${cplpref}bcdtoa.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/ustring/astring/instoa.c /Fo:${dest_obj}/${cplpref}instoa.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/ustring/astring/MemHeap.c /Fo:${dest_obj}/${cplpref}MemHeap.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/ustring/astring/StrHeap.c /Fo:${dest_obj}/${cplpref}StrHeap.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/ustring/astring/StrHeapAppend.c /Fo:${dest_obj}/${cplpref}StrHeapAppend.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/ustring/astring/StrHeapAppendChars.c /Fo:${dest_obj}/${cplpref}StrHeapAppendChars.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/ustring/astring/StrHeapAppendN.c /Fo:${dest_obj}/${cplpref}StrHeapAppendN.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/ustring/astring/StrHeapFromChar.c /Fo:${dest_obj}/${cplpref}StrHeapFromChar.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/ustring/astring/StrHeapInsertThrow.c /Fo:${dest_obj}/${cplpref}StrHeapInsertThrow.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/ustring/astring/StrHeapN.c /Fo:${dest_obj}/${cplpref}StrHeapN.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/ustring/astring/StrReplace.c /Fo:${dest_obj}/${cplpref}StrReplace.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/ustring/chrar/ChrAdd.c /Fo:${dest_obj}/${cplpref}ChrAdd.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/ustring/chrar/ChrArrange.c /Fo:${dest_obj}/${cplpref}ChrArrange.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/ustring/chrar/ChrCmp.c /Fo:${dest_obj}/${cplpref}ChrCmp.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/ustring/chrar/ChrCombinate.c /Fo:${dest_obj}/${cplpref}ChrCombinate.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/ustring/chrar/ChrComDiv.c /Fo:${dest_obj}/${cplpref}ChrComDiv.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/ustring/chrar/ChrComMul.c /Fo:${dest_obj}/${cplpref}ChrComMul.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/ustring/chrar/ChrCpz.c /Fo:${dest_obj}/${cplpref}ChrCpz.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/ustring/chrar/ChrCtz.c /Fo:${dest_obj}/${cplpref}ChrCtz.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/ustring/chrar/ChrDecToHex.c /Fo:${dest_obj}/${cplpref}ChrDecToHex.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/ustring/chrar/ChrDecToHexFloat.c /Fo:${dest_obj}/${cplpref}ChrDecToHexFloat.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/ustring/chrar/ChrDiv.c /Fo:${dest_obj}/${cplpref}ChrDiv.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/ustring/chrar/ChrFactorial.c /Fo:${dest_obj}/${cplpref}ChrFactorial.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/ustring/chrar/ChrHexToDec.c /Fo:${dest_obj}/${cplpref}ChrHexToDec.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/ustring/chrar/ChrHexToDecFloat.c /Fo:${dest_obj}/${cplpref}ChrHexToDecFloat.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/ustring/chrar/ChrInsPow.c /Fo:${dest_obj}/${cplpref}ChrInsPow.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/ustring/chrar/ChrMul.c /Fo:${dest_obj}/${cplpref}ChrMul.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/ustring/chrar/ChrSub.c /Fo:${dest_obj}/${cplpref}ChrSub.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/ustring/chrar/DigDec.c /Fo:${dest_obj}/${cplpref}DigDec.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/ustring/chrar/DigInc.c /Fo:${dest_obj}/${cplpref}DigInc.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/ustring/gstring/atoins.c /Fo:${dest_obj}/${cplpref}atoins.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/ustring/gstring/instob.c /Fo:${dest_obj}/${cplpref}instob.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/ustring/gstring/MemCompareRight.c /Fo:${dest_obj}/${cplpref}MemCompareRight.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/ustring/gstring/MemSet.c /Fo:${dest_obj}/${cplpref}MemSet.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/ustring/gstring/StrDeprefixSpaces.c /Fo:${dest_obj}/${cplpref}StrDeprefixSpaces.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/ustring/gstring/StrDesuffixSpaces.c /Fo:${dest_obj}/${cplpref}StrDesuffixSpaces.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/ustring/gstring/StrFilter.c /Fo:${dest_obj}/${cplpref}StrFilter.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/ustring/gstring/StrFilterOut.c /Fo:${dest_obj}/${cplpref}StrFilterOut.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/ustring/gstring/StrFilterOutString.c /Fo:${dest_obj}/${cplpref}StrFilterOutString.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/ustring/gstring/StrFilterString.c /Fo:${dest_obj}/${cplpref}StrFilterString.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/ustring/gstring/StrShiftLeft4.c /Fo:${dest_obj}/${cplpref}StrShiftLeft4.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/ustring/gstring/StrShiftLeft8n.c /Fo:${dest_obj}/${cplpref}StrShiftLeft8n.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/ustring/gstring/StrShiftRight4.c /Fo:${dest_obj}/${cplpref}StrShiftRight4.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/ustring/gstring/StrShiftRight8n.c /Fo:${dest_obj}/${cplpref}StrShiftRight8n.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/ustring/gstring/StrSortBubble.c /Fo:${dest_obj}/${cplpref}StrSortBubble.obj /I${VI_64} ${attr}
	$(CC) /c ./lib/c/ustring/gstring/_size_decimal_get.c /Fo:${dest_obj}/${cplpref}_size_decimal_get.obj /I${VI_64} ${attr}
	$(CX) /c ./lib/cpp/coe.cpp /Fo:${dest_obj}/${cpppref}coe.obj /I${VI_64} ${attr}
	$(CX) /c ./lib/cpp/consio.cpp /Fo:${dest_obj}/${cpppref}consio.obj /I${VI_64} ${attr}
	$(CX) /c ./lib/cpp/debug.cpp /Fo:${dest_obj}/${cpppref}debug.obj /I${VI_64} ${attr}
	$(CX) /c ./lib/cpp/floating.cpp /Fo:${dest_obj}/${cpppref}floating.obj /I${VI_64} ${attr}
	$(CX) /c ./lib/cpp/numberx.cpp /Fo:${dest_obj}/${cpppref}numberx.obj /I${VI_64} ${attr}
	$(CX) /c ./lib/cpp/stream.cpp /Fo:${dest_obj}/${cpppref}stream.obj /I${VI_64} ${attr}
	$(CX) /c ./lib/cpp/string.cpp /Fo:${dest_obj}/${cpppref}string.obj /I${VI_64} ${attr}
	$(CX) /c ./lib/cpp/strpool.cpp /Fo:${dest_obj}/${cpppref}strpool.obj /I${VI_64} ${attr}
	$(CX) /c ./lib/cpp/nodes/dnode.cpp /Fo:${dest_obj}/${cpppref}dnode.obj /I${VI_64} ${attr}
	$(CX) /c ./lib/cpp/nodes/inode.cpp /Fo:${dest_obj}/${cpppref}inode.obj /I${VI_64} ${attr}
	$(CX) /c ./lib/cpp/nodes/nnode.cpp /Fo:${dest_obj}/${cpppref}nnode.obj /I${VI_64} ${attr}
	$(CX) /c ./lib/cpp/nodes/node.cpp /Fo:${dest_obj}/${cpppref}node.obj /I${VI_64} ${attr}
	$(CX) /c ./lib/cpp/nodes/tnode.cpp /Fo:${dest_obj}/${cpppref}tnode.obj /I${VI_64} ${attr}
	$(CX) /c ./lib/cpp/nodes/nnode/NnodeParse.cpp /Fo:${dest_obj}/${cpppref}NnodeParse.obj /I${VI_64} ${attr}
	$(CX) /c ./lib/cpp/nodes/nnode/NnodeParse/NnodeParseOperator.cpp /Fo:${dest_obj}/${cpppref}NnodeParseOperator.obj /I${VI_64} ${attr}
	$(CX) /c ./lib/cpp/nodes/tnode/TokenParse.cpp /Fo:${dest_obj}/${cpppref}TokenParse.obj /I${VI_64} ${attr}
	${AR} /OUT:${dest_abs} ${dest_obj}/*
