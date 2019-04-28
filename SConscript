Import('RTT_ROOT')
Import('rtconfig')
from building import *

cwd = GetCurrentDir()
CPPPATH = [cwd]

src = Split('''
        ipmsg.c
        ''')

if GetDepend('IPMSG_MSGRS_ENABLE'):
    src += ['examples/msgrs.c']
    if GetDepend('SOC_W600_A8xx'):
        src += ['examples/wm_fwup.c']

if GetDepend('IPMSG_FILERECV_ENABLE'):
    src += ['ipmsg_filerecv.c']

group = DefineGroup('ipmsg', src, depend = ['PKG_USING_IPMSG'], CPPPATH = CPPPATH)

Return('group')
