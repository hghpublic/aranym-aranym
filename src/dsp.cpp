/* Joy 2001 */

#include "sysdeps.h"
#include "hardware.h"
#include "cpu_emulation.h"
#include "memory.h"
#include "dsp.h"
#include "dsp_cpu.h"

#define DEBUG 1
#include "debug.h"

void DSP::init(void)
{
	int i;

	memset(ram, 0,sizeof(ram));
	memset(periph, 0,sizeof(periph));
	memset(stack, 0,sizeof(stack));
	memset(registers, 0,sizeof(registers));
	/* FIXME : initialize rom[] */

	state = DSP_BOOTING;
	D(bug("Dsp: booting"));
	bootstrap_pos = bootstrap_accum = 0;
	
	/* Registers */
	pc = 0x0000;
	registers[REG_OMR]=0x02;
	for (i=0;i<8;i++) {
		registers[REG_M0+i]=0x00ffff;
	}

	/* host port init, dsp side */
	periph[SPACE_X][PERIPH_HOST_HSR]=(1<<HOST_HSR_HTDE);

	/* host port init, cpu side */
	periph[SPACE_X][CPU_HOST_CVR]=0x12;
	periph[SPACE_X][CPU_HOST_ISR]=(1<<HOST_ISR_TRDY)|(1<<HOST_ISR_TXDE);
	periph[SPACE_X][CPU_HOST_IVR]=0x0f;

	/* Misc */
	loop_rep = 0;
}

uae_u8 DSP::handleRead(uaecptr addr)
{
	uae_u8 value=0;

	addr -= HW_DSP;

	switch(addr) {
		case CPU_HOST_ICR:
			value = periph[SPACE_X][CPU_HOST_ICR];
			break;
		case CPU_HOST_CVR:
			value = periph[SPACE_X][CPU_HOST_CVR];
			break;
		case CPU_HOST_ISR:
			value = periph[SPACE_X][CPU_HOST_ISR];
			break;
		case CPU_HOST_IVR:
			value = periph[SPACE_X][CPU_HOST_IVR];
			break;
		case CPU_HOST_RX0:
			value = 0;
			break;
		case CPU_HOST_RXH:
			value = (periph[SPACE_X][PERIPH_HOST_HRX]>>16) & 255;
			break;
		case CPU_HOST_RXM:
			value = (periph[SPACE_X][PERIPH_HOST_HRX]>>8) & 255;
			break;
		case CPU_HOST_RXL:
			value = periph[SPACE_X][PERIPH_HOST_HRX] & 255;
#if 0
			/* Set HTDE bit to say that CPU has read */
			periph[SPACE_X][PERIPH_HOST_HSR] |= 1<<HOST_HSR_HTDE;
			/* Clear RXDF bit to say that CPU has read */
			periph[SPACE_X][CPU_HOST_ISR] &= 0xff-(1<<HOST_ISR_RXDF);
#endif
			break;
	}

	D(bug("HWget_b(0x%08x)=0x%02x at 0x%08x", addr+HW_DSP, value, showPC()));
	return value;
}

void DSP::handleWrite(uaecptr addr, uae_u8 value)
{
	addr -= HW_DSP;

	switch(addr) {
		case CPU_HOST_ICR:
			periph[SPACE_X][CPU_HOST_ICR]=value & 0xfb;
			break;
		case CPU_HOST_CVR:
			periph[SPACE_X][CPU_HOST_CVR]=value & 0x9f;
			/* if bit 7=1, host command */
			break;
		case CPU_HOST_ISR:
			/* Read only */
			break;
		case CPU_HOST_IVR:
			periph[SPACE_X][CPU_HOST_IVR]=value;
			break;
		case CPU_HOST_TX0:
			bootstrap_accum = 0;
			break;
		case CPU_HOST_TXH:
			bootstrap_accum |= value<<16;
			break;
		case CPU_HOST_TXM:
			bootstrap_accum |= value<<8;
			break;
		case CPU_HOST_TXL:
			bootstrap_accum |= value;

			switch(state) {
				case DSP_BOOTING:
					ram[SPACE_P][bootstrap_pos] = bootstrap_accum;
					bootstrap_pos++;
					if (bootstrap_pos == 0x200) {
						D(bug("Dsp: bootstrap done"));
						state = DSP_RUNNING;
					}		
					bootstrap_accum = 0;
					break;
				case DSP_WAITHOST:
					/* Wake up DSP if a waiting data arrived */
				case DSP_RUNNING:
					/* New data available on host port */
				case DSP_WAITINTERRUPT:
					/* Wake up if host port interrupt */
					periph[SPACE_X][PERIPH_HOST_HTX]=bootstrap_accum;
					state = DSP_RUNNING;
					D(bug("Writing 0x%06x on host port, from 0x%08x", bootstrap_accum, showPC()));
					break;
			}
#if 0
			/* Set HRDF bit to say that DSP can read */
			periph[SPACE_X][PERIPH_HOST_HSR] |= 1<<HOST_HSR_HRDF;
			/* Clear TXDE bit to say that CPU has written */
			periph[SPACE_X][CPU_HOST_ISR] &= 0xff-(1<<HOST_ISR_TXDE);
#endif
			break;
	}

/*	D(bug("HWput_b(0x%08x,0x%02x) at 0x%08x", addr+HW_DSP, value, showPC()));*/
}
