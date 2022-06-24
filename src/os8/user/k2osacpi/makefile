#   
#   BSD 3-Clause License
#   
#   Copyright (c) 2020, Kurt Kennett
#   All rights reserved.
#   
#   Redistribution and use in source and binary forms, with or without
#   modification, are permitted provided that the following conditions are met:
#   
#   1. Redistributions of source code must retain the above copyright notice, this
#      list of conditions and the following disclaimer.
#   
#   2. Redistributions in binary form must reproduce the above copyright notice,
#      this list of conditions and the following disclaimer in the documentation
#      and/or other materials provided with the distribution.
#   
#   3. Neither the name of the copyright holder nor the names of its
#      contributors may be used to endorse or promote products derived from
#      this software without specific prior written permission.
#   
#   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
#   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
#   FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
#   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
#   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
#   CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
#   OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

include $(K2_ROOT)/src/$(K2_OS)/build/k2ospre.make

TARGET_TYPE = XDL

#
# Link to kernel
#
XDL_INF := k2osacpi.inf

GCCOPT += -I $(K2_ROOT)/src/$(K2_OS)/user/k2osacpi/acpica/source/include
GCCOPT += -I $(K2_ROOT)/src/$(K2_OS)/user/k2osacpi/acpica/source/include/platform
GCCOPT += -DACPI_USE_NATIVE_DIVIDE

#
# ACPICA adaptation 
#

SOURCES += xdl_entry.c 

SOURCES += cache.c
SOURCES += initterm.c
SOURCES += intr.c
SOURCES += lock.c
SOURCES += map.c
SOURCES += mem.c
SOURCES += override.c
SOURCES += port.c
SOURCES += printf.c
SOURCES += root.c
SOURCES += rwmem.c
SOURCES += sem.c
SOURCES += signal.c
SOURCES += thread.c
SOURCES += timer.c
SOURCES += sleep.c 
SOURCES += pciconfig.c
SOURCES += run.c
SOURCES += mutex.c
SOURCES += enum.c 
SOURCES += methods.c

#
# ACPICA SOURCES
#

# dispatcher 
SOURCES += acpica/source/components/dispatcher/dsargs.c
SOURCES += acpica/source/components/dispatcher/dscontrol.c 
SOURCES += acpica/source/components/dispatcher/dsdebug.c 
SOURCES += acpica/source/components/dispatcher/dsfield.c 
SOURCES += acpica/source/components/dispatcher/dsinit.c 
SOURCES += acpica/source/components/dispatcher/dsmethod.c
SOURCES += acpica/source/components/dispatcher/dsmthdat.c 
SOURCES += acpica/source/components/dispatcher/dsobject.c 
SOURCES += acpica/source/components/dispatcher/dsopcode.c 
SOURCES += acpica/source/components/dispatcher/dspkginit.c 
SOURCES += acpica/source/components/dispatcher/dsutils.c 
SOURCES += acpica/source/components/dispatcher/dswexec.c 
SOURCES += acpica/source/components/dispatcher/dswload.c 
SOURCES += acpica/source/components/dispatcher/dswload2.c 
SOURCES += acpica/source/components/dispatcher/dswscope.c 
SOURCES += acpica/source/components/dispatcher/dswstate.c 

# common
SOURCES += acpica/source/common/ahids.c
SOURCES += acpica/source/common/ahpredef.c

# events
SOURCES += acpica/source/components/events/evevent.c
SOURCES += acpica/source/components/events/evglock.c
SOURCES += acpica/source/components/events/evgpe.c
SOURCES += acpica/source/components/events/evgpeblk.c
SOURCES += acpica/source/components/events/evgpeinit.c
SOURCES += acpica/source/components/events/evgpeutil.c
SOURCES += acpica/source/components/events/evhandler.c
SOURCES += acpica/source/components/events/evmisc.c
SOURCES += acpica/source/components/events/evregion.c
SOURCES += acpica/source/components/events/evrgnini.c
SOURCES += acpica/source/components/events/evsci.c
SOURCES += acpica/source/components/events/evxface.c
SOURCES += acpica/source/components/events/evxfevnt.c
SOURCES += acpica/source/components/events/evxfgpe.c
SOURCES += acpica/source/components/events/evxfregn.c

# executer
SOURCES += acpica/source/components/executer/exconcat.c
SOURCES += acpica/source/components/executer/exconfig.c
SOURCES += acpica/source/components/executer/exconvrt.c
SOURCES += acpica/source/components/executer/excreate.c
SOURCES += acpica/source/components/executer/exdebug.c
SOURCES += acpica/source/components/executer/exdump.c
SOURCES += acpica/source/components/executer/exfield.c
SOURCES += acpica/source/components/executer/exfldio.c
SOURCES += acpica/source/components/executer/exmisc.c
SOURCES += acpica/source/components/executer/exmutex.c
SOURCES += acpica/source/components/executer/exnames.c
SOURCES += acpica/source/components/executer/exoparg1.c
SOURCES += acpica/source/components/executer/exoparg2.c
SOURCES += acpica/source/components/executer/exoparg3.c
SOURCES += acpica/source/components/executer/exoparg6.c
SOURCES += acpica/source/components/executer/exprep.c
SOURCES += acpica/source/components/executer/exregion.c
SOURCES += acpica/source/components/executer/exresnte.c
SOURCES += acpica/source/components/executer/exresolv.c
SOURCES += acpica/source/components/executer/exresop.c
SOURCES += acpica/source/components/executer/exserial.c
SOURCES += acpica/source/components/executer/exstore.c
SOURCES += acpica/source/components/executer/exstoren.c
SOURCES += acpica/source/components/executer/exstorob.c
SOURCES += acpica/source/components/executer/exsystem.c
SOURCES += acpica/source/components/executer/extrace.c
SOURCES += acpica/source/components/executer/exutils.c

# hardware
SOURCES += acpica/source/components/hardware/hwacpi.c
SOURCES += acpica/source/components/hardware/hwesleep.c
SOURCES += acpica/source/components/hardware/hwgpe.c
SOURCES += acpica/source/components/hardware/hwpci.c
SOURCES += acpica/source/components/hardware/hwregs.c
SOURCES += acpica/source/components/hardware/hwsleep.c
SOURCES += acpica/source/components/hardware/hwtimer.c
SOURCES += acpica/source/components/hardware/hwvalid.c
SOURCES += acpica/source/components/hardware/hwxface.c
SOURCES += acpica/source/components/hardware/hwxfsleep.c

# namespace 
SOURCES += acpica/source/components/namespace/nsaccess.c
SOURCES += acpica/source/components/namespace/nsalloc.c
SOURCES += acpica/source/components/namespace/nsarguments.c
SOURCES += acpica/source/components/namespace/nsconvert.c
SOURCES += acpica/source/components/namespace/nsdump.c
SOURCES += acpica/source/components/namespace/nsdumpdv.c
SOURCES += acpica/source/components/namespace/nseval.c
SOURCES += acpica/source/components/namespace/nsinit.c
SOURCES += acpica/source/components/namespace/nsload.c
SOURCES += acpica/source/components/namespace/nsnames.c
SOURCES += acpica/source/components/namespace/nsobject.c
SOURCES += acpica/source/components/namespace/nsparse.c
SOURCES += acpica/source/components/namespace/nspredef.c
SOURCES += acpica/source/components/namespace/nsprepkg.c
SOURCES += acpica/source/components/namespace/nsrepair.c
SOURCES += acpica/source/components/namespace/nsrepair2.c
SOURCES += acpica/source/components/namespace/nssearch.c
SOURCES += acpica/source/components/namespace/nsutils.c
SOURCES += acpica/source/components/namespace/nswalk.c
SOURCES += acpica/source/components/namespace/nsxfeval.c
SOURCES += acpica/source/components/namespace/nsxfname.c
SOURCES += acpica/source/components/namespace/nsxfobj.c

# parser 
SOURCES += acpica/source/components/parser/psargs.c
SOURCES += acpica/source/components/parser/psloop.c
SOURCES += acpica/source/components/parser/psobject.c
SOURCES += acpica/source/components/parser/psopcode.c
SOURCES += acpica/source/components/parser/psopinfo.c
SOURCES += acpica/source/components/parser/psparse.c
SOURCES += acpica/source/components/parser/psscope.c
SOURCES += acpica/source/components/parser/pstree.c
SOURCES += acpica/source/components/parser/psutils.c
SOURCES += acpica/source/components/parser/pswalk.c
SOURCES += acpica/source/components/parser/psxface.c

# resources 
SOURCES += acpica/source/components/resources/rsaddr.c
SOURCES += acpica/source/components/resources/rscalc.c
SOURCES += acpica/source/components/resources/rscreate.c
#SOURCES += acpica/source/components/resources/rsdump.c
SOURCES += acpica/source/components/resources/rsdumpinfo.c
SOURCES += acpica/source/components/resources/rsinfo.c
SOURCES += acpica/source/components/resources/rsio.c
SOURCES += acpica/source/components/resources/rsirq.c
SOURCES += acpica/source/components/resources/rslist.c
SOURCES += acpica/source/components/resources/rsmemory.c
SOURCES += acpica/source/components/resources/rsmisc.c
SOURCES += acpica/source/components/resources/rsserial.c
SOURCES += acpica/source/components/resources/rsutils.c
SOURCES += acpica/source/components/resources/rsxface.c

# tables
SOURCES += acpica/source/components/tables/tbdata.c
SOURCES += acpica/source/components/tables/tbfadt.c
SOURCES += acpica/source/components/tables/tbfind.c
SOURCES += acpica/source/components/tables/tbinstal.c
SOURCES += acpica/source/components/tables/tbprint.c
SOURCES += acpica/source/components/tables/tbutils.c
SOURCES += acpica/source/components/tables/tbxface.c
SOURCES += acpica/source/components/tables/tbxfload.c
SOURCES += acpica/source/components/tables/tbxfroot.c

# utilities 
SOURCES += acpica/source/components/utilities/utaddress.c
SOURCES += acpica/source/components/utilities/utalloc.c
SOURCES += acpica/source/components/utilities/utascii.c
SOURCES += acpica/source/components/utilities/utbuffer.c
SOURCES += acpica/source/components/utilities/utcache.c
SOURCES += acpica/source/components/utilities/utclib.c
SOURCES += acpica/source/components/utilities/utcopy.c
SOURCES += acpica/source/components/utilities/utdebug.c
SOURCES += acpica/source/components/utilities/utdecode.c
SOURCES += acpica/source/components/utilities/utdelete.c
SOURCES += acpica/source/components/utilities/uterror.c
SOURCES += acpica/source/components/utilities/uteval.c
SOURCES += acpica/source/components/utilities/utexcep.c
SOURCES += acpica/source/components/utilities/utglobal.c
SOURCES += acpica/source/components/utilities/uthex.c
SOURCES += acpica/source/components/utilities/utids.c
SOURCES += acpica/source/components/utilities/utinit.c
SOURCES += acpica/source/components/utilities/utlock.c
SOURCES += acpica/source/components/utilities/utmath.c
SOURCES += acpica/source/components/utilities/utmisc.c
SOURCES += acpica/source/components/utilities/utmutex.c
SOURCES += acpica/source/components/utilities/utnonansi.c
SOURCES += acpica/source/components/utilities/utobject.c
SOURCES += acpica/source/components/utilities/utosi.c
SOURCES += acpica/source/components/utilities/utownerid.c
SOURCES += acpica/source/components/utilities/utpredef.c
SOURCES += acpica/source/components/utilities/utprint.c
SOURCES += acpica/source/components/utilities/utresdecode.c
SOURCES += acpica/source/components/utilities/utresrc.c
SOURCES += acpica/source/components/utilities/utstate.c
SOURCES += acpica/source/components/utilities/utstring.c
SOURCES += acpica/source/components/utilities/utstrsuppt.c
SOURCES += acpica/source/components/utilities/utstrtoul64.c
SOURCES += acpica/source/components/utilities/uttrack.c
SOURCES += acpica/source/components/utilities/utuuid.c
SOURCES += acpica/source/components/utilities/utxface.c
SOURCES += acpica/source/components/utilities/utxferror.c
SOURCES += acpica/source/components/utilities/utxfinit.c
SOURCES += acpica/source/components/utilities/utxfmutex.c

ifeq ($(K2_ARCH),X32)
STATIC_LIBS += @shared/lib/k2archx32
endif

ifeq ($(K2_ARCH),A32)
STATIC_LIBS += @shared/lib/k2archa32
endif

include $(K2_ROOT)/src/build/post.make

