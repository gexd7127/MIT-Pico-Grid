#include "F28x_Project.h"
#include "Comm.h"


//***********************************************************************//
//                     G l o b a l  V a r i a b l e s                    //
//***********************************************************************//

Uint16 IPC_rx[SIZEOFIPC_RX] = {0};
Uint16 IPC_tx[SIZEOFIPC_TX] = {0};

extern Uint16 RS485_tx[SIZEOFRS485_TX];
extern Uint16 RS485_rx[SIZEOFRS485_RX];

//***********************************************************************//
//                          F u n c t i o n s                            //
//***********************************************************************//
void IPC_RX(Uint16 * array)
{
    IPC_rx[0] = array[0];
    IPC_rx[1] = array[1];
}

void IPC_TX(Uint16 * array)
{
    array[0] = RS485_rx[0];
    array[1] = RS485_rx[1];
	IpcRegs.IPCSET.bit.IPC0 = 1; //Set the remote CPU interrupt
}

void Reset_remote_IPC(void)
{
	IpcRegs.IPCACK.bit.IPC0 = 1; //clear the remote CPU interrupt
}

void Wait_memory_access(void)
{
    while(!( MemCfgRegs.GSxMSEL.bit.MSEL_GS0 &
 		    MemCfgRegs.GSxMSEL.bit.MSEL_GS14 ) )
    {
    }
}

void IPC_TO_RS485Interpreter(void)
{
    Uint16 i;
    for(i=0;i<SIZEOFRS485_TX/2;i++)
    {
        RS485_tx[2*i] = IPC_rx[i];
        RS485_tx[2*i+1] = IPC_rx[i]/256;
    }
}