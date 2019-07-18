#include "F28x_Project.h"
#include <stdint.h>
#include <math.h>
#include "..\Test_setting.h"
#include "..\Peripheral\Peripheral.h"
#include "Meas.h"
#include "control.h"

//***********************************************************************//
//                     G l o b a l  V a r i a b l e s                    //
//***********************************************************************//
extern struct_meas_states meas_states1, meas_states2;
struct_control_states control_states1, control_states2;

//***********************************************************************//
//                          F u n c t i o n s                            //
//***********************************************************************//
#pragma CODE_SECTION(Control_step, "ramfuncs")
void Control_step(const float32 Droop[2], const bool enable)
{
	static float32 VC_PID_states1[2] = {0, 0}, IL_PID_states1[2] = {0, 0};
	static float32 VC_PID_states2[2] = {0, 0}, IL_PID_states2[2] = {0, 0};

	Droop_control(enable, Droop, &control_states1, &meas_states1);
	Virtual_component(enable, &control_states1, &meas_states1);
	VC_control(enable, &control_states1, &meas_states1, VC_PID_states1);
	IL_control(enable, &control_states1, &meas_states1, IL_PID_states1);

	Droop_control(enable, Droop, &control_states2, &meas_states2);
	Virtual_component(enable, &control_states2, &meas_states2);
	VC_control(enable, &control_states2, &meas_states2, VC_PID_states2);
	IL_control(enable, &control_states2, &meas_states2, IL_PID_states2);

//	control_states1.VINV_dq[0] = control_states1.VC_dq_ref[0];
//	control_states1.VINV_dq[1] = control_states1.VC_dq_ref[1];
//
//	control_states2.VINV_dq[0] = control_states2.VC_dq_ref[0];
//	control_states2.VINV_dq[1] = control_states2.VC_dq_ref[1];

	VINV2Duty(&control_states1, &meas_states1);
	VINV2Duty(&control_states2, &meas_states2);

}
//#pragma CODE_SECTION(Droop_control, "ramfuncs")
void Droop_control(const bool enable, const float32 Droop[2], struct_control_states * c_states, struct_meas_states * m_states)
{
	c_states->omega = W_NOM - (enable? Droop[0] * W_NOM * m_states->PQ[0] : 0);
	c_states->VC_dq_ref[0] = V_NOM - (enable? Droop[1] * V_NOM * m_states->PQ[1]: 0);
	c_states->VC_dq_ref[1] = 0;

	c_states->omega= (c_states->omega >  W_NOM*1.2?  W_NOM*1.2 : c_states->omega);
	c_states->omega= (c_states->omega <  W_NOM*0.8?  W_NOM*0.8 : c_states->omega);
}

//#pragma CODE_SECTION(Virtual_component, "ramfuncs")
void Virtual_component(const bool enable, struct_control_states * c_states, struct_meas_states * m_states)
{
	if (enable)
	{
		// Virtual X
		c_states->VC_dq_ref[0] = c_states->VC_dq_ref[0] + XM * m_states->IO_dq[1];
		c_states->VC_dq_ref[1] = c_states->VC_dq_ref[1] - XM * m_states->IO_dq[0];
		// Virtual L
		c_states->LPF_outL[0] = LPF(c_states->LPF_outL[0], WF, m_states->IO_dq[0]);
		c_states->LPF_outL[1] = LPF(c_states->LPF_outL[1], WF, m_states->IO_dq[1]);
		c_states->VC_dq_ref[0] = c_states->VC_dq_ref[0] - LM*WF*(m_states->IO_dq[0]-c_states->LPF_outL[0]);
		c_states->VC_dq_ref[1] = c_states->VC_dq_ref[1] - LM*WF*(m_states->IO_dq[1]-c_states->LPF_outL[1]);
		// Virtual C
		c_states->LPF_outC[0] = LPF(c_states->LPF_outC[0], WRC, m_states->IO_dq[0]);
		c_states->LPF_outC[1] = LPF(c_states->LPF_outC[1], WRC, m_states->IO_dq[1]);
		c_states->VC_dq_ref[0] = c_states->VC_dq_ref[0] - RS*c_states->LPF_outC[0];
		c_states->VC_dq_ref[1] = c_states->VC_dq_ref[1] - RS*c_states->LPF_outC[1];
	}
	else
	{
		c_states->LPF_outL[0] = 0;
		c_states->LPF_outL[1] = 0;
		c_states->LPF_outC[0] = 0;
		c_states->LPF_outC[1] = 0;
	}
}


#pragma CODE_SECTION(VC_control, "ramfuncs")
void VC_control(const bool enable, struct_control_states * c_states, struct_meas_states * m_states, float32 PID_states[2])
{
	float32 ff_dq[2] = {0}, error[2] = {0};
	Uint16 i;

	ff_dq[0] = m_states->IO_dq[0]*VC_FF_GAIN - c_states->omega*CF*m_states->VC_dq[1];
	ff_dq[1] = m_states->IO_dq[1]*VC_FF_GAIN + c_states->omega*CF*m_states->VC_dq[0];

	for (i = 0;i<=1;i++) error[i] = c_states->VC_dq_ref[i] - m_states->VC_dq[i];
	PID_dq(c_states->IL_dq_ref, PID_states, error, KPV, KIV);

	// Reset PID integrator
	if (enable == false){
		for (i = 0;i<=1;i++) PID_states[i] = 0;
	}

	for (i = 0;i<=1;i++) c_states->IL_dq_ref[i] += ff_dq[i];

//	for (i = 0;i<=1;i++){
//		c_states->IL_dq_ref[i] = (c_states->IL_dq_ref[i] >  25.0f?  25.0f : c_states->IL_dq_ref[i]);
//		c_states->IL_dq_ref[i] = (c_states->IL_dq_ref[i] < -25.0f? -25.0f : c_states->IL_dq_ref[i]);
//	}
}


#pragma CODE_SECTION(IL_control, "ramfuncs")
void IL_control(const bool enable, struct_control_states * c_states, struct_meas_states * m_states, float32 PID_states[2])
{
	float32 ff_dq[2] = {0}, error[2] = {0};
	Uint16 i;

	ff_dq[0] = m_states->VC_dq[0]*IL_FF_GAIN - c_states->omega*LF*m_states->IL_dq[1];
	ff_dq[1] = m_states->VC_dq[1]*IL_FF_GAIN + c_states->omega*LF*m_states->IL_dq[0];

	for (i = 0;i<=1;i++) error[i] = c_states->IL_dq_ref[i] - m_states->IL_dq[i];
	PID_dq(c_states->VINV_dq, PID_states, error, KPC, KIC);

	// Reset PID integrator
	if (enable == false)
	{
		for (i = 0;i<=1;i++) PID_states[i] = 0;
	}

	for (i = 0;i<=1;i++) c_states->VINV_dq[i] += ff_dq[i];

//	for (i = 0;i<=1;i++){
//		c_states->VINV_dq[i] = (c_states->VINV_dq[i] >  V_NOM*1.2f?  V_NOM*1.2f : c_states->VINV_dq[i]);
////		c_states->VINV_dq[i] = (c_states->VINV_dq[i] <  -V_NOM*1.2f?  -V_NOM*1.2f : c_states->VINV_dq[i]);
//	}

}

#pragma CODE_SECTION(PID_dq, "ramfuncs")
void PID_dq(float32 out[2], float32 PID_states[2], const float32 error[2], const float32 kp, const float32 ki)
{
	Uint16 i;
	for (i = 0;i<=1;i++)
	{
		PID_states[i] += ki*ISR_PERIOD*error[i];
		out[i] = kp*error[i] + PID_states[i];
	}
}

#pragma CODE_SECTION(VINV2Duty, "ramfuncs")
void VINV2Duty (struct_control_states * c_states, struct_meas_states * m_states)
{
	float32 abc[3];
	dq2abc(abc, c_states->VINV_dq, m_states->theta);

	c_states->Duty[0] = (Uint16)( (abc[0]/VDC + 0.5f) * PWM_PERIOD);
	c_states->Duty[1] = (Uint16)( (abc[1]/VDC + 0.5f) * PWM_PERIOD);
	c_states->Duty[2] = (Uint16)( (abc[2]/VDC + 0.5f) * PWM_PERIOD);

//	control_states.Duty[0] = (Uint16)( 0.5f* (0.8f*sinf(meas_states.theta) + 1.0f) * PWM_PERIOD);
//	control_states.Duty[1] = (Uint16)( 0.5f* (0.8f*sinf(meas_states.theta-PHASE_120) + 1.0f) * PWM_PERIOD);
//	control_states.Duty[2] = (Uint16)( 0.5f* (0.8f*sinf(meas_states.theta+PHASE_120) + 1.0f) * PWM_PERIOD);

}

#pragma CODE_SECTION(dq2abc, "ramfuncs")
void dq2abc(float32 abc[3], const float32 dq[2], const float32 theta)
{
	abc[0] = dq[0]*sinf(theta) + dq[1]*cosf(theta);
	abc[1] = dq[0]*sinf(theta-PHASE_120) + dq[1]*cosf(theta-PHASE_120);
	abc[2] = dq[0]*sinf(theta+PHASE_120) + dq[1]*cosf(theta+PHASE_120);
}

