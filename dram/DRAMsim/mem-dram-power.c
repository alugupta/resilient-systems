/*
 * mem-dram-power.c - All power functions are now included in this file.
 *
 * This is released as part of the sim-dram infrastructure for power/timing
 * simulation of the memory system. Sim-dram can run stand-alone or be
 * integrated with any standard CPU simulator.
 *
 * Copyright (c) 2004 by David Wang Brinda Ganesh Bruce Jacob
 * Brinda Ganesh, David Wang, Bruce Jacob
 * Systems and Computer Architecture Lab
 * Dept of Electrical & Computer Engineering
 * University of Maryland, College Park
 * All Rights Reserved
 *
 * This software is distributed with *ABSOLUTELY NO SUPPORT* and
 * *NO WARRANTY*.  Permission is given to modify this code
 * as long as this notice is not removed.
 *
 * Send feedback to Brinda Ganesh brinda@eng.umd.edu
 * 				 or David Wang davewang@wam.umd.edu
 *               or Bruce Jacob blj@eng.umd.edu
 */


#ifndef MEMORY_SYSTEM_H
#include "mem-system.h"
#endif

#define MEM_POWER_GLOBAL 0
#define MEM_POWER_PERIODIC 1

void calculate_power_values(power_config_t * power_config_ptr);

tick_t get_command_completion_time(command_t* this_c) ;
void initialize_power_config(power_config_t * power_config_ptr){
  power_config_ptr->max_VDD = 2.7;
  power_config_ptr->min_VDD = 2.3;
  power_config_ptr->VDD = 2.5;
  power_config_ptr->P_per_DQ = 0;
  power_config_ptr->density = 128;
  power_config_ptr->DQ = 8;
  power_config_ptr->DQS = 1;
  power_config_ptr->IDD0 = 0;
  power_config_ptr->IDD2P = 0;
  power_config_ptr->IDD2F = 0;
  power_config_ptr->IDD3P = 0;
  power_config_ptr->IDD3N = 0;
  power_config_ptr->IDD4R = 0;
  power_config_ptr->IDD4W = 0;
  power_config_ptr->IDD5 = 0;
  power_config_ptr->t_CK = 5.0;
  power_config_ptr->t_RC = 55.0;
  power_config_ptr->t_RFC_min = 105.0;
  power_config_ptr->t_REFI = 7.8;
  power_config_ptr->print_period = 400;

  calculate_power_values(power_config_ptr);
}
void calculate_power_values(power_config_t * power_config_ptr){
  power_config_ptr->VDD_scale = ( power_config_ptr->VDD* power_config_ptr->VDD)/( power_config_ptr->max_VDD* power_config_ptr->max_VDD);
  power_config_ptr->freq_scale = ( power_config_ptr->t_CK * (float) get_dram_frequency()/2)/1000.0;  /*** FIX ME : use_freq/spec_freq***/

  power_config_ptr->p_PRE_PDN = power_config_ptr->IDD2P * power_config_ptr->max_VDD *power_config_ptr->VDD_scale;
  power_config_ptr->p_PRE_STBY = power_config_ptr->IDD2F * power_config_ptr->max_VDD * power_config_ptr->VDD_scale * power_config_ptr->freq_scale;
  power_config_ptr->p_ACT_PDN = power_config_ptr->IDD3P * power_config_ptr->max_VDD * power_config_ptr->VDD_scale;
  power_config_ptr->p_ACT_STBY = power_config_ptr->IDD3N * power_config_ptr->max_VDD * power_config_ptr->VDD_scale * power_config_ptr->freq_scale;
  power_config_ptr->p_WR = (power_config_ptr->IDD4W - power_config_ptr->IDD3N) * power_config_ptr->max_VDD * power_config_ptr->VDD_scale * power_config_ptr->freq_scale;
  power_config_ptr->p_RD = (power_config_ptr->IDD4R - power_config_ptr->IDD3N) * power_config_ptr->max_VDD *power_config_ptr->VDD_scale * power_config_ptr->freq_scale;
  power_config_ptr->p_DQ = power_config_ptr->P_per_DQ * (power_config_ptr->DQ + power_config_ptr->DQS) * power_config_ptr->freq_scale;
  power_config_ptr->p_REF = (power_config_ptr->IDD5 - power_config_ptr->IDD3N) * power_config_ptr->max_VDD *power_config_ptr->VDD_scale * power_config_ptr->freq_scale;
  dram_system_configuration_t *pConfig = get_dram_system_config();
  if (get_dram_type() == DDRSDRAM)
    power_config_ptr->p_ACT = (float)(power_config_ptr->IDD0 - power_config_ptr->IDD3N) * power_config_ptr->max_VDD * power_config_ptr->VDD_scale*power_config_ptr->freq_scale;
  else // DDR2
    power_config_ptr->p_ACT = (float)(power_config_ptr->IDD0 -
        ((power_config_ptr->IDD3N * (float) pConfig->t_ras +
          power_config_ptr->IDD2F * (float)(pConfig->t_rc - pConfig->t_ras))
         /pConfig->t_rc)) * power_config_ptr->max_VDD * power_config_ptr->VDD_scale*power_config_ptr->freq_scale;

  return;
}
void read_power_config_from_file(FILE * fp, power_config_t * power_config_ptr){
  char *ret;
  char line[512];
   if (fp == NULL){
      fprintf(stderr,"Invalid Power Configuration File Pointer\n");
      exit (0);
    }
    //Read the DRAM characteristic from the file
    printf("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< \n");
    printf("ABOUT TO READ DRAM CONFIG FILE\n");
    printf("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< \n");
    do {
      ret = fgets(line, sizeof(line)-1, fp);
      line[sizeof(line) - 1 ] = '\0';
      int len = strlen(line);
      if(len>0)
        line[len-1] = '\0';
      if((ret!=NULL) && strlen(line)) {
        if (line[0] != '#' ) {
	  //parse the parameters here!
		  if (!strncmp(line,"density",7)) {
			sscanf(line, "density %d", &power_config_ptr->density);
			if (power_config_ptr->density < 1)
			  fprintf(stderr,"density must be greater than zero.");
		  }else if (!strncmp(line,"chip_count",10)) {
			sscanf(line, "chip_count %d", &power_config_ptr->chip_count);
			if (power_config_ptr->chip_count < 1)
			  fprintf(stderr,"Chip Count must be greater than zero.");
		  }else if (!strncmp(line,"DQS",3)) {
			sscanf(line, "DQS %d", &power_config_ptr->DQS);
			if (power_config_ptr->DQS < 1)
			  fprintf(stderr,"DQS must be greater than zero.");
		  } else if (!strncmp(line,"max_VDD",7)) {
			sscanf(line, "max_VDD %f", &power_config_ptr->max_VDD);
			if (power_config_ptr->max_VDD <= 0)
			  fprintf(stderr,"max_VDD must be greater than zero.");
		  } else if (!strncmp(line,"min_VDD",7)) {
			sscanf(line, "min_VDD %f", &power_config_ptr->min_VDD);
			if ((power_config_ptr->min_VDD <= 0) || (power_config_ptr->min_VDD > power_config_ptr->max_VDD))
			  fprintf(stderr,"min_VDD must be greater than zero and smaller than max_VDD.");
		  }else if (!strncmp(line,"IDD0",4)) {
			sscanf(line, "IDD0 %d", &power_config_ptr->IDD0);
			if (power_config_ptr->IDD0 < 1)
			  fprintf(stderr,"IDD0 must be greater than zero.");
		  }else if (!strncmp(line,"IDD2P",5)) {
			sscanf(line, "IDD2P %d", &power_config_ptr->IDD2P);
			if (power_config_ptr->IDD2P < 1)
			  fprintf(stderr,"IDD2P must be greater than zero.");
		  }else if (!strncmp(line,"IDD2F",5)) {
			sscanf(line, "IDD2F %d", &power_config_ptr->IDD2F);
			if (power_config_ptr->IDD2F < 1)
			  fprintf(stderr,"IDD2F must be greater than zero.");
		  } else if (!strncmp(line,"IDD3P",5)) {
			sscanf(line, "IDD3P %d", &power_config_ptr->IDD3P);
			if (power_config_ptr->IDD3P < 1)
			  fprintf(stderr,"IDD3P must be greater than zero.");
		  }else if (!strncmp(line,"IDD3N",5)) {
			sscanf(line, "IDD3N %d", &power_config_ptr->IDD3N);
			if (power_config_ptr->IDD3N < 1)
			  fprintf(stderr,"IDD3N must be greater than zero.");
		  }else if (!strncmp(line,"IDD4R",5)) {
			sscanf(line, "IDD4R %d", &power_config_ptr->IDD4R);
			if (power_config_ptr->IDD4R < 1)
			  fprintf(stderr,"IDD4R must be greater than zero.");
		  } else if (!strncmp(line,"IDD4W",5)) {
			sscanf(line, "IDD4W %d", &power_config_ptr->IDD4W);
			if (power_config_ptr->IDD4W < 1)
			  fprintf(stderr,"IDD4W must be greater than zero.");
		  } else if (!strncmp(line,"IDD5",4)) {
			sscanf(line, "IDD5 %d", &power_config_ptr->IDD5);
			if (power_config_ptr->IDD5 < 1)
			  fprintf(stderr,"IDD5 must be greater than zero.");
		  }else if (!strncmp(line,"t_CK",4)) {
			sscanf(line, "t_CK %f", &power_config_ptr->t_CK);
			if (power_config_ptr->t_CK < 1.0)
			  fprintf(stderr,"t_CK must be greater than zero.");
		  }else if (!strncmp(line,"VDD",3)) {
			sscanf(line, "VDD %f", &power_config_ptr->VDD);
			if (power_config_ptr->VDD <= 0)
			  fprintf(stderr,"VDD must be greater than zero.");
		  }else if (!strncmp(line,"P_per_DQ",8)) {
			sscanf(line, "P_per_DQ %f", &power_config_ptr->P_per_DQ);
			if (power_config_ptr->P_per_DQ <= 0 )
			  fprintf(stderr,"P_per_DQ must be greater than zero.");
		  }else if (!strncmp(line,"t_RFC_min",9)) {
			sscanf(line, "t_RFC_min %f", &power_config_ptr->t_RFC_min);
			if (power_config_ptr->t_RFC_min <= 0 )
			  fprintf(stderr,"t_RFC_min must be greater than zero.");
		  }else if (!strncmp(line,"t_REFI",7)) {
			sscanf(line, "t_REFI %f", &power_config_ptr->t_REFI);
			if (power_config_ptr->P_per_DQ <= 0 )
			  fprintf(stderr,"t_REFI must be greater than zero.");
		  }else if (!strncmp(line,"ICC_Idle_2",10)) {
			sscanf(line, "ICC_Idle_2 %f", &power_config_ptr->ICC_Idle_2);
			if (power_config_ptr->ICC_Idle_2 <= 0 )
		  		fprintf(stderr,"ICC_Idle_2 must be greater than zero.");
        }else if (!strncmp(line,"ICC_Idle_1",10)) {
			sscanf(line, "ICC_Idle_1 %f", &power_config_ptr->ICC_Idle_1);
			if (power_config_ptr->ICC_Idle_1 <= 0 )
		  		fprintf(stderr,"ICC_Idle_1 must be greater than zero.");
        }else if (!strncmp(line,"ICC_Idle_0",10)) {
			sscanf(line, "ICC_Idle_0 %f", &power_config_ptr->ICC_Idle_0);
			if (power_config_ptr->ICC_Idle_0 <= 0 )
		  		fprintf(stderr,"ICC_Idle_0 must be greater than zero.");
        }else if (!strncmp(line,"ICC_Active_1",12)) { /* Active */
			sscanf(line, "ICC_Active_1 %f", &power_config_ptr->ICC_Active_1);
			if (power_config_ptr->ICC_Active_1 <= 0 )
		  		fprintf(stderr,"ICC_Active_1 must be greater than zero.");
        }else if (!strncmp(line,"ICC_Active_2",12)){ /* Data Pass through */
			sscanf(line, "ICC_Active_2 %f", &power_config_ptr->ICC_Active_2);
			if (power_config_ptr->ICC_Active_2 <= 0 )
		  		fprintf(stderr,"ICC_Active_2 must be greater than zero.");
        }else if (!strncmp(line,"VCC",3)){ /* AMB Voltage */
			sscanf(line, "VCC %f", &power_config_ptr->VCC);
			if (power_config_ptr->VCC <= 0 )
		  		fprintf(stderr,"VCC must be greater than zero.");
        }





      }
	  }
    } while (!feof(fp));

  //calculate_power_values(power_config_ptr);

}

void dump_power_config(FILE *stream) {
  dram_system_configuration_t * config = get_dram_system_config();
  power_config_t* power_config_ptr = &(config->dram_power_config);
  fprintf(stream,"  Operating Current: One Bank Active-Precharge : IDD0 %d\n",power_config_ptr->IDD0);
  fprintf(stream,"  Precharge Power Down Current : IDD2P %d\n",power_config_ptr->IDD2P);
  fprintf(stream,"  Precharge Standby Current : IDD3P %d\n",power_config_ptr->IDD3P);
  fprintf(stream,"  Active Power Down Current : IDD2F %d\n",power_config_ptr->IDD2F);
  fprintf(stream,"  Active Standby Current : IDD3N %d\n",power_config_ptr->IDD3N);
  fprintf(stream,"  Operating Burst Read  Current : IDD4R %d\n",power_config_ptr->IDD4R);
  fprintf(stream,"  Operating Burst Write Current : IDD4W %d\n",power_config_ptr->IDD4W);
  fprintf(stream,"  Operating Burst Refresh Current : IDD5 %d\n",power_config_ptr->IDD5);
  fprintf(stream,"  t_RFC_min: %f ns\n",power_config_ptr->t_RFC_min);

  if (config->dram_type == FBD_DDR2) {
    fprintf(stream,"  Idle Current, single or last DIMM L0 state : ICC_Idle_0 %f\n",power_config_ptr->ICC_Idle_0);
    fprintf(stream,"  Idle Current, first DIMM L0 state : ICC_Idle_1 %f\n",power_config_ptr->ICC_Idle_1);
    fprintf(stream,"  Idle Current, DRAM power down L0 state : ICC_Idle_2 %f\n",power_config_ptr->ICC_Idle_2);
    fprintf(stream,"  Active Power L0 state : ICC_Active_1 %f\n",power_config_ptr->ICC_Active_1);
    fprintf(stream,"  Active Power L0 state : ICC_Active_2 %f\n",power_config_ptr->ICC_Active_2);

  }
  fprintf(stream,"  p_PRE_PDN   %f\n",power_config_ptr->p_PRE_PDN);
  fprintf(stream,"  p_PRE_STBY  %f\n",power_config_ptr->p_PRE_STBY);
  fprintf(stream,"  p_ACT_PDN   %f\n",power_config_ptr->p_ACT_PDN);
  fprintf(stream,"  p_ACT_STBY  %f\n",power_config_ptr->p_ACT_STBY);
  fprintf(stream,"  p_ACT       %f\n",power_config_ptr->p_ACT);
  fprintf(stream,"  p_WR        %f\n",power_config_ptr->p_WR);
  fprintf(stream,"  p_RD        %f\n",power_config_ptr->p_RD);
  fprintf(stream,"  p_REF       %f\n",power_config_ptr->p_REF);
  fprintf(stream,"  p_DQ        %f\n",power_config_ptr->p_DQ);
}

/*
 * This function essentially computes the Power Consumption of a given rank on
 * a given channel
 * duration for you to select whether you want global or periodic power info!
 */

void do_power_calc(tick_t now,
	dram_system_t *dram_system,
	int chan_id,
	int rank_id,
	power_meas_t *meas_ptr,
	int type
	){
  dram_system_configuration_t * config = &(dram_system->config);
  power_config_t* power_config_ptr = &(config->dram_power_config);
  power_info_t * dram_power = &dram_system->dram_power;
  float sum_WR_percent = 0;
  float sum_RD_percent = 0;
  int j;
  power_counter_t *r_p_info_ptr ;
  if (type == MEM_POWER_GLOBAL) {
    r_p_info_ptr = &dram_system->dram_controller[chan_id].rank[rank_id].r_p_gblinfo;
    // We need to take care of making sure that n_ACT is bounded
  }
  else
    r_p_info_ptr = &dram_system->dram_controller[chan_id].rank[rank_id].r_p_info;

  //Avoiding divided by zero
  if (r_p_info_ptr->RAS_count == 0)
    r_p_info_ptr->RAS_count = 1;

  if ((config->dram_type == DDR2 || config->dram_type == FBD_DDR2) ) {
    //Calculate sum of RD/WR for the whole DRAM system
    for(j = 0 ; j < config->rank_count ; j++) {
      if (type == MEM_POWER_GLOBAL) {
        sum_WR_percent += (float) dram_system->dram_controller[chan_id].rank[j].r_p_gblinfo.sum_per_WR/
          (float)(dram_system->dram_controller[chan_id].rank[j].r_p_gblinfo.RAS_sum > 0?
                  dram_system->dram_controller[chan_id].rank[j].r_p_gblinfo.RAS_sum:1); //Ohm: change RAS_count to RAS_sum
        sum_RD_percent += (float) dram_system->dram_controller[chan_id].rank[j].r_p_gblinfo.sum_per_RD/
          (float)(dram_system->dram_controller[chan_id].rank[j].r_p_gblinfo.RAS_sum > 0?
                  dram_system->dram_controller[chan_id].rank[j].r_p_gblinfo.RAS_sum:1);
      }else {
        sum_WR_percent += (float) dram_system->dram_controller[chan_id].rank[j].r_p_info.sum_per_WR/
          (float)(dram_system->dram_controller[chan_id].rank[j].r_p_gblinfo.RAS_sum > 0?
                  dram_system->dram_controller[chan_id].rank[j].r_p_info.RAS_sum:1);
        sum_RD_percent += (float) dram_system->dram_controller[chan_id].rank[j].r_p_info.sum_per_RD/
          (float)(dram_system->dram_controller[chan_id].rank[j].r_p_gblinfo.RAS_sum > 0?
                  dram_system->dram_controller[chan_id].rank[j].r_p_info.RAS_sum:1);
      }
    }
  }


      // Power Calculation
      // Activate Power
      // In memory clock cycles RAS_sum = # of cycles that RAS has been active
      //
      meas_ptr->t_ACT =(float)r_p_info_ptr->RAS_sum/((float) r_p_info_ptr->RAS_count);

      if (meas_ptr->t_ACT == 0.0)
        meas_ptr->p_ACT = 0; // No ACT issued in the last print period
      else {
        if (config->dram_type == DDRSDRAM)
          //		  meas_ptr->p_ACT = (float)power_config_ptr->p_ACT * (power_config_ptr->t_RC/(float)(power_config_ptr->t_CK * meas_ptr->t_ACT));
          meas_ptr->p_ACT = (float)power_config_ptr->p_ACT * (config->t_rc/(float)(meas_ptr->t_ACT));

        else // DDR2
          //	  		meas_ptr->p_ACT = (float)power_config_ptr->p_ACT * (power_config_ptr->t_RC/(float)(power_config_ptr->t_CK * meas_ptr->t_ACT));
          meas_ptr->p_ACT = (float)power_config_ptr->p_ACT * (config->t_rc/(float)(meas_ptr->t_ACT));
      }
      // Background Power
      meas_ptr->CKE_LO_PRE = 1.0 - ((float) r_p_info_ptr->cke_hi_pre/(float) (r_p_info_ptr->bnk_pre>0?r_p_info_ptr->bnk_pre: 1));
	 if (type == MEM_POWER_PERIODIC) {
      meas_ptr->CKE_LO_ACT = 1.0 - ((float) r_p_info_ptr->cke_hi_act/
		(float) ((now - dram_power->last_print_time) - r_p_info_ptr->bnk_pre));

      meas_ptr->BNK_PRE = (float) r_p_info_ptr->bnk_pre/
        ( (float)(now - dram_power->last_print_time));

      //meas_ptr->BNK_PRE = (float)  r_p_info_ptr->bnk_pre/((float)r_p_info_ptr->RAS_sum);
	 }else {
      meas_ptr->CKE_LO_ACT = 1.0 - ((float) r_p_info_ptr->cke_hi_act/
		(float) (now   - r_p_info_ptr->bnk_pre));

      meas_ptr->BNK_PRE = (float) r_p_info_ptr->bnk_pre/ ( (float)(now));

      //meas_ptr->BNK_PRE = (float) r_p_info_ptr->bnk_pre/( (float)(r_p_info_ptr->RAS_sum));
	 }
	 if (meas_ptr->CKE_LO_PRE < 0)
	   meas_ptr->CKE_LO_PRE = 0;
	 if (meas_ptr->CKE_LO_ACT < 0)
	   meas_ptr->CKE_LO_ACT = 0;

      meas_ptr->p_PRE_PDN = power_config_ptr->p_PRE_PDN * meas_ptr->BNK_PRE * meas_ptr->CKE_LO_PRE;
      meas_ptr->p_PRE_STBY = power_config_ptr->p_PRE_STBY * meas_ptr->BNK_PRE * (1.0 - meas_ptr->CKE_LO_PRE);
	  meas_ptr->p_PRE_MAX = power_config_ptr->p_PRE_STBY * meas_ptr->BNK_PRE * 1.0; /* The max if there was no power down */
      meas_ptr->p_ACT_PDN = power_config_ptr->p_ACT_PDN * (1-meas_ptr->BNK_PRE) * meas_ptr->CKE_LO_ACT;
      meas_ptr->p_ACT_STBY = power_config_ptr->p_ACT_STBY * (1-meas_ptr->BNK_PRE) * (1.0 - meas_ptr->CKE_LO_ACT);
      meas_ptr->p_ACT_MAX = power_config_ptr->p_ACT_STBY * (1-meas_ptr->BNK_PRE) * (1.0);
      // Read/Write Power
      if (r_p_info_ptr->RAS_sum > 0) {
        meas_ptr->WR_percent = (float) r_p_info_ptr->sum_per_WR/ (float) r_p_info_ptr->RAS_sum;
        meas_ptr->RD_percent = (float) r_p_info_ptr->sum_per_RD/ (float) r_p_info_ptr->RAS_sum;
      }
      else {
        meas_ptr->WR_percent = 0;
        meas_ptr->RD_percent = 0;
      }

      meas_ptr->p_WR = power_config_ptr->p_WR * meas_ptr->WR_percent;
      meas_ptr->p_RD = power_config_ptr->p_RD * meas_ptr->RD_percent;
      meas_ptr->p_DQ = power_config_ptr->p_DQ * meas_ptr->RD_percent;

      meas_ptr->p_REF = 0, meas_ptr->p_TOT = 0, meas_ptr->p_TOT_MAX = 0;

      //reset p_AMB to zero to prevent printing junk in non-AMB DRAM
      meas_ptr->p_AMB = 0;
      meas_ptr->p_AMB_IDLE = 0;
      meas_ptr->p_AMB_ACT = 0;
      meas_ptr->p_AMB_PASS_THROUGH = 0;

      if (config->dram_type == DDRSDRAM) {
	//ignore the refresh power for now
        //p_REF = (IDD5 - IDD2P) * max_VDD * VDD_scale;
        meas_ptr->p_REF = power_config_ptr->p_REF * r_p_info_ptr->refresh_count;

        meas_ptr->p_TOT = meas_ptr->p_PRE_PDN + meas_ptr->p_PRE_STBY + meas_ptr->p_ACT_PDN + meas_ptr->p_ACT_STBY + meas_ptr->p_ACT + meas_ptr->p_WR + meas_ptr->p_RD + meas_ptr->p_DQ + meas_ptr->p_REF;
	  // At this point, we have to total power per chip
	  // need to calculate the total power of eack rank
  	  int chip_count = (config->channel_width * 8)/power_config_ptr->DQ; //number of chips per rank FIXME
	  meas_ptr->p_TOT *= chip_count; // Total power for a rank
	  meas_ptr->p_TOT_MAX *= chip_count; // Total power for a rank
      }
	  else if (config->dram_type == DDR2 ) {
		//ignore the refresh power for now
		//p_REF = (IDD5 - IDD3N) * max_VDD * t_RFC_min/(t_REFI * 1000) * VDD_scale; // Assume auto-refresh
		float p_dq_RD = 0;
		float p_dq_WR = 0;
		float p_dq_RDoth = 0;
		float p_dq_WRoth = 0;
		float termRDsch = 0;
		float termWRsch = 0;

		if (config->rank_count == 1) { /* FIXME What about 2/3? */
		  p_dq_RD = 1.1;
		  p_dq_WR = 8.2;

		} else { // rank_count == 4
		  p_dq_RD = 1.5;
		  p_dq_WR = 0;
		  p_dq_RDoth = 13.1;
		  p_dq_WRoth = 14.6;

		  //calculate termRDsch and terWRsch
		  // *** FIXME : I don't know how to calculate this ***
		  // Average teh RD/WR percent of the other DRAMs
		  termRDsch = (sum_RD_percent - meas_ptr->RD_percent)/(config->channel_count*config->rank_count - 1);
		  termWRsch = (sum_WR_percent - meas_ptr->WR_percent)/(config->channel_count*config->rank_count - 1);
		}
		// Let's calculate the DQ and termination power!
		meas_ptr->p_DQ = p_dq_RD * (power_config_ptr->DQ + power_config_ptr->DQS) * meas_ptr->RD_percent;
		float p_termW = p_dq_WR * (power_config_ptr->DQ + power_config_ptr->DQS + 1) * meas_ptr->WR_percent;
		float p_termRoth = p_dq_RDoth * (power_config_ptr->DQ + power_config_ptr->DQS) * termRDsch;
		float p_termWoth = p_dq_WRoth * (power_config_ptr->DQ + power_config_ptr->DQS + 1) * termWRsch;

      if (r_p_info_ptr->refresh_count >0) {
        if (type == MEM_POWER_PERIODIC) {
          meas_ptr->p_REF = power_config_ptr->p_REF * (float) power_config_ptr->t_RFC_min/((now-dram_power->last_print_time)/((config->memory_frequency / 1000.0)*r_p_info_ptr->refresh_count));
        }else {
          meas_ptr->p_REF = power_config_ptr->p_REF * (float) power_config_ptr->t_RFC_min/((now)/((config->memory_frequency / 1000.0)*r_p_info_ptr->refresh_count));

        }
      }

		meas_ptr->p_TOT = meas_ptr->p_PRE_PDN + meas_ptr->p_PRE_STBY + meas_ptr->p_ACT_PDN + meas_ptr->p_ACT_STBY + meas_ptr->p_ACT + meas_ptr->p_WR + meas_ptr->p_RD + meas_ptr->p_DQ + meas_ptr->p_REF + p_termW + p_termRoth + p_termWoth;
		meas_ptr->p_TOT_MAX = meas_ptr->p_PRE_MAX + meas_ptr->p_ACT_MAX + meas_ptr->p_ACT + meas_ptr->p_WR + meas_ptr->p_RD + meas_ptr->p_DQ + meas_ptr->p_REF + p_termW + p_termRoth + p_termWoth;
	  // At this point, we have to total power per chip
	  // need to calculate the total power of eack rank
  	  int chip_count = (config->channel_width * 8)/power_config_ptr->DQ; //number of chips per rank FIXME
	  meas_ptr->p_TOT *= chip_count; // Total power for a rank
	  meas_ptr->p_TOT_MAX *= chip_count; // Total power for a rank

	  } else if (config->dram_type == FBD_DDR2) {
        /* FIXME How do we take care of termination and DQ ? */
		/* FIXME Paramterize these assuming one rank per dimm */
		float p_dq_RD = 1.1;
		float p_dq_WR = 8.2;
		meas_ptr->p_DQ = p_dq_RD * (power_config_ptr->DQ + power_config_ptr->DQS) * meas_ptr->RD_percent + p_dq_WR * (power_config_ptr->DQ + power_config_ptr->DQS + 1) * meas_ptr->WR_percent;
      if (r_p_info_ptr->refresh_count >0) {
        if (type == MEM_POWER_PERIODIC) {
          meas_ptr->p_REF = power_config_ptr->p_REF * (float) power_config_ptr->t_RFC_min/((now-dram_power->last_print_time)/((config->memory_frequency / 1000.0)*r_p_info_ptr->refresh_count));
        }else {
          meas_ptr->p_REF = power_config_ptr->p_REF * (float) power_config_ptr->t_RFC_min/((now)/((config->memory_frequency / 1000.0)*r_p_info_ptr->refresh_count));
        }
      }
		/* Calculate p_AMB */
        // Note that if the system is running using traces we need to bound
        // amb_busy and amb_data_pass_through
        if (r_p_info_ptr->amb_busy > now)
          r_p_info_ptr->amb_busy = now;
        if (r_p_info_ptr->amb_data_pass_through> now)
          r_p_info_ptr->amb_data_pass_through = now;

		meas_ptr->p_AMB_ACT = power_config_ptr->VCC * power_config_ptr->ICC_Active_1 * r_p_info_ptr->amb_busy * 1000;
	   	meas_ptr->p_AMB_PASS_THROUGH = power_config_ptr->VCC * power_config_ptr->ICC_Active_2*r_p_info_ptr->amb_data_pass_through  *1000; /*ICC is in A */

		if (type == MEM_POWER_PERIODIC) {
		  if (rank_id == config->rank_count - 1) {
		  	meas_ptr->p_AMB_IDLE = power_config_ptr->VCC * power_config_ptr->ICC_Idle_0*(now - dram_power->last_print_time - r_p_info_ptr->amb_busy - r_p_info_ptr->amb_data_pass_through ) *1000; /*ICC is in A */
		  }else {
			  meas_ptr->p_AMB_IDLE = power_config_ptr->VCC * power_config_ptr->ICC_Idle_1*(now - dram_power->last_print_time - r_p_info_ptr->amb_busy - r_p_info_ptr->amb_data_pass_through ) *1000; /*ICC is in A */
		  }
		  meas_ptr->p_AMB = meas_ptr->p_AMB_IDLE + meas_ptr->p_AMB_ACT + meas_ptr->p_AMB_PASS_THROUGH; /*ICC is in A */
		  meas_ptr->p_AMB = meas_ptr->p_AMB/(now - dram_power->last_print_time);
		  meas_ptr->p_AMB_IDLE /= (now - dram_power->last_print_time);
		  meas_ptr->p_AMB_ACT /= (now - dram_power->last_print_time);
		  meas_ptr->p_AMB_PASS_THROUGH /= (now - dram_power->last_print_time);
		}else {
		  if (rank_id == config->rank_count - 1) {
		  	meas_ptr->p_AMB_IDLE = power_config_ptr->VCC * power_config_ptr->ICC_Idle_0*(now - r_p_info_ptr->amb_busy - r_p_info_ptr->amb_data_pass_through ) *1000; /*ICC is in A */
		  }else {
		  	meas_ptr->p_AMB_IDLE = power_config_ptr->VCC * power_config_ptr->ICC_Idle_1*(now - r_p_info_ptr->amb_busy - r_p_info_ptr->amb_data_pass_through ) *1000; /*ICC is in A */
		  }
		  meas_ptr->p_AMB = meas_ptr->p_AMB_IDLE + meas_ptr->p_AMB_ACT + meas_ptr->p_AMB_PASS_THROUGH; /*ICC is in A */
		  meas_ptr->p_AMB = meas_ptr->p_AMB/(now);
		  meas_ptr->p_AMB_IDLE = meas_ptr->p_AMB_IDLE/(now);
		  meas_ptr->p_AMB_ACT /= (now);
		  meas_ptr->p_AMB_PASS_THROUGH /= (now);
		}

		meas_ptr->p_TOT = meas_ptr->p_PRE_PDN + meas_ptr->p_PRE_STBY + meas_ptr->p_ACT_PDN + meas_ptr->p_ACT_STBY + meas_ptr->p_ACT + meas_ptr->p_WR + meas_ptr->p_RD + meas_ptr->p_DQ;
		meas_ptr->p_TOT_MAX = meas_ptr->p_PRE_MAX + meas_ptr->p_ACT_MAX + meas_ptr->p_ACT + meas_ptr->p_WR + meas_ptr->p_RD + meas_ptr->p_DQ;
		// At this point, we have to total power per chip
		// need to calculate the total power of eack rank
		meas_ptr->p_TOT *= power_config_ptr->chip_count; // Total power for a rank
		meas_ptr->p_TOT_MAX *= power_config_ptr->chip_count; // Total power for a rank
		meas_ptr->p_TOT += meas_ptr->p_AMB;
		meas_ptr->p_TOT_MAX += meas_ptr->p_AMB;
	  }else {
		meas_ptr->p_TOT = 0; // not support other types of DRAM
	  }



}
//Ohm--Add 3 functions
//print_power_stats() : calculate and print power every a period of time.
//check_cke_hi_pre() : check if the current tick is in precharge state with CKE enable
//check_cke_hi_act(): check if the current tick is in active state with CKE enable
void print_power_stats(tick_t now, dram_system_t *dram_system) {
  //FILE *fp;
  int chan_id, rank_id;
  dram_system_configuration_t * config = &(dram_system->config);
  power_config_t* power_config_ptr = &(config->dram_power_config);
  power_info_t * dram_power = &dram_system->dram_power;
  int chip_count = (config->channel_width * 8)/power_config_ptr->DQ; //number of chips per rank FIXME

  if (now < dram_power->last_print_time + config->dram_power_config.print_period ||
	  dram_power->power_file_ptr == NULL ){
	return;
  }
#ifdef DEBUG_POWER
  fprintf(stdout,"\n===================print_power_stats====================\n");
#endif
  //assert (dram_power->power_file_ptr != NULL);
  /* This stuff should not be calculated every time fix it */
   power_config_ptr->t_RC = (float) (config->t_rc * power_config_ptr->t_CK / 2); // spec t_RC in nanosecond unit FIXME
      power_config_ptr->DQ = power_config_ptr->density * 1024 * 1024/
        (config->row_count * config->bank_count * config->col_count); /* FIXME : need to do calculation during configure not all the time */

  for (chan_id = 0; chan_id < config->channel_count; chan_id++) {
    for (rank_id = 0; rank_id < config->rank_count; rank_id++) {
        //printf("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< \n");
        //printf("CALCULATING POWER STATS for chan_id %d and rank %d\n", chan_id, rank_id);
        //printf("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< \n");
	  power_meas_t meas_tmp;
	  do_power_calc(now,dram_system,chan_id,rank_id,&meas_tmp,MEM_POWER_PERIODIC);
      if (rank_id==0)
        fprintf(dram_power->power_file_ptr,"%llu    ", now);
      else
        fprintf(dram_power->power_file_ptr,"            ");


	  fprintf(dram_power->power_file_ptr,"ch[%d] ra[%d] p_PRE_PDN[%5.2f] p_PRE_STBY[%5.2f] p_ACT_PDN[%5.2f]  p_ACT_STBY[%5.2f] p_ACT[%5.2f] p_WR[%5.2f] p_RD[%5.2f]  p_REF[%5.2f] p_AMB_IDLE[%5.2f] p_AMB_ACT[%5.2f] p_AMB_PASS_THROUGH[%5.2f] p_AMB[%5.2f] p_TOT[%5.2f] P_TOT_MAX[%5.2f] access[%d] \n",chan_id,rank_id, meas_tmp.p_PRE_PDN,meas_tmp.p_PRE_STBY,meas_tmp.p_ACT_PDN,meas_tmp.p_ACT_STBY,meas_tmp.p_ACT,meas_tmp.p_WR,meas_tmp.p_RD, meas_tmp.p_REF,meas_tmp.p_AMB_IDLE,meas_tmp.p_AMB_ACT,meas_tmp.p_AMB_PASS_THROUGH,meas_tmp.p_AMB,meas_tmp.p_TOT,meas_tmp.p_TOT_MAX, dram_system->dram_controller[chan_id].rank[rank_id].r_p_info.dram_access);



	}
	fprintf(dram_power->power_file_ptr,"\n");
  }
  for (chan_id = 0; chan_id < config->channel_count; chan_id++) {
    for (rank_id = 0; rank_id < config->rank_count; rank_id++) {
	  dram_system->dram_controller[chan_id].rank[rank_id].r_p_info.bnk_pre = 0;
	  dram_system->dram_controller[chan_id].rank[rank_id].r_p_info.cke_hi_pre = 0;
	  dram_system->dram_controller[chan_id].rank[rank_id].r_p_info.cke_hi_act = 0;
	  //dram_system->dram_controller[chan_id].rank[rank_id].last_RAS_time = 0;
	  dram_system->dram_controller[chan_id].rank[rank_id].r_p_info.RAS_count = 0;
	  dram_system->dram_controller[chan_id].rank[rank_id].r_p_info.RAS_sum = 0;
	  dram_system->dram_controller[chan_id].rank[rank_id].r_p_info.sum_per_RD = 0;
	  dram_system->dram_controller[chan_id].rank[rank_id].r_p_info.sum_per_WR = 0;
	  dram_system->dram_controller[chan_id].rank[rank_id].r_p_info.dram_access=0;
	  dram_system->dram_controller[chan_id].rank[rank_id].r_p_info.amb_busy= dram_system->dram_controller[chan_id].rank[rank_id].r_p_info.amb_busy_spill_over;
	  dram_system->dram_controller[chan_id].rank[rank_id].r_p_info.amb_data_pass_through= dram_system->dram_controller[chan_id].rank[rank_id].r_p_info.amb_data_pass_through_spill_over;
	  dram_system->dram_controller[chan_id].rank[rank_id].r_p_info.amb_busy_spill_over= 0;
	  dram_system->dram_controller[chan_id].rank[rank_id].r_p_info.amb_data_pass_through_spill_over= 0;
	  dram_system->dram_controller[chan_id].rank[rank_id].r_p_info.refresh_count = 0;
    }
  }
  dram_power->last_print_time = now;
}

int check_cke_hi_pre(int chan_id, int rank_id) {
  //At this point, the rank  should be in precharge state!
  return 	dram_system.dram_controller[chan_id].rank[rank_id].cke_bit;
}

int check_cke_hi_act(int chan_id, int rank_id) {

  return 	dram_system.dram_controller[chan_id].rank[rank_id].cke_bit;

}

void update_cke_bit(tick_t now,
	int chan_id,
	int max_ranks) {
	int i;
	for (i=0;i<max_ranks;i++) {
	  if (dram_system.dram_controller[chan_id].rank[i].cke_done < now)
		 dram_system.dram_controller[chan_id].rank[i].cke_bit = false;
	}
	return;
}

void mem_initialize_power_collection(power_info_t *dram_power, char *filename) {
  if (filename != NULL) {
	if ((dram_power->power_file_ptr = fopen(filename,"w")) == NULL) {
	  fprintf(stderr,"Cannot open power stats file %s\n",filename);
	  exit (0);
	}
  }else {
	dram_power->power_file_ptr = stdout;
  }
  dram_power->last_print_time = 0;
}

void power_update_freq_based_values(int freq,power_config_t *power_config_ptr){
       power_config_ptr->freq_scale = ( power_config_ptr->t_CK * (float) get_dram_frequency()/2)/1000.0;  /*** FIX ME : use_freq/spec_freq***/
}

void print_global_power_stats(FILE *fileout) {
  int chan_id, rank_id;
  dram_system_configuration_t * config = get_dram_system_config();
  power_config_t* power_config_ptr = &(config->dram_power_config);
  tick_t now = get_dram_current_time();
  dram_system_t *dram_system = get_dram_system();

  /* This stuff should not be calculated every time fix it */
   power_config_ptr->t_RC = (float) (config->t_rc * power_config_ptr->t_CK / 2); // spec t_RC in nanosecond unit FIXME
      power_config_ptr->DQ = power_config_ptr->density * 1024 * 1024/
        (config->row_count * config->bank_count * config->col_count); /* FIXME : need to do calculation during configure not all the time */
	  if (get_dram_type() == DDRSDRAM)
          power_config_ptr->p_ACT = (float)(power_config_ptr->IDD0 - power_config_ptr->IDD3N) * power_config_ptr->max_VDD * power_config_ptr->VDD_scale;
        else // DDR2
	  		power_config_ptr->p_ACT = (float)(power_config_ptr->IDD0 -
                          ((power_config_ptr->IDD3N * (float) config->t_ras +
			    power_config_ptr->IDD2F * (float)(config->t_rc - config->t_ras))
                           /config->t_rc)) * power_config_ptr->max_VDD * power_config_ptr->VDD_scale;
  fprintf(fileout," ---------- POWER STATS ------------------------\n");
  for (chan_id = 0; chan_id < config->channel_count; chan_id++) {
    for (rank_id = 0; rank_id < config->rank_count; rank_id++) {
	  power_meas_t meas_tmp;
	  do_power_calc(now,dram_system,chan_id,rank_id,&meas_tmp,MEM_POWER_GLOBAL);
      if (rank_id==0)
        fprintf(fileout,"%llu    ", now);
      else
        fprintf(fileout,"            ");

	  fprintf(fileout,"ch[%d] ra[%d] p_PRE_PDN[%5.2f] p_PRE_STBY[%5.2f] p_ACT_PDN[%5.2f]  p_ACT_STBY[%5.2f] p_ACT[%5.2f] p_WR[%5.2f] p_RD[%5.2f] p_REF[%5.2f] p_AMB_IDLE[%5.2f] p_AMB_ACT[%5.2f] p_AMB_PASS_THROUGH[%5.2f] p_AMB[%5.2f] p_TOT[%5.2f] P_TOT_MAX[%5.2f] access[%d]\n",chan_id,rank_id, meas_tmp.p_PRE_PDN,meas_tmp.p_PRE_STBY,meas_tmp.p_ACT_PDN,meas_tmp.p_ACT_STBY,meas_tmp.p_ACT,meas_tmp.p_WR,meas_tmp.p_RD, meas_tmp.p_REF,meas_tmp.p_AMB_IDLE,meas_tmp.p_AMB_ACT,meas_tmp.p_AMB_PASS_THROUGH,meas_tmp.p_AMB,meas_tmp.p_TOT,meas_tmp.p_TOT_MAX, dram_system->dram_controller[chan_id].rank[rank_id].r_p_gblinfo.dram_access);


	}
	fprintf(fileout,"\n");
  }
}

/* Is rank busy ? AMB busy => true
 * Is rank not busy ? Bundle issued => AMB data pass through
 */
void update_amb_power_stats(command_t *bundle[],int command_count,tick_t now) {
  dram_system_t *sys_ptr = get_dram_system();

  int i;
  tick_t completion_time;
  int chan_id = bundle[0]->chan_id;
  tick_t dram_power_print_time = sys_ptr->dram_power.last_print_time + sys_ptr->config.dram_power_config.print_period;

  /* Update busy till */
  for (i=0;i<command_count && bundle[i] != NULL;i++) {
    completion_time = get_command_completion_time(bundle[i]);
    if (completion_time > sys_ptr->dram_controller[bundle[i]->chan_id].rank[bundle[i]->rank_id].r_p_info.busy_till) {
      sys_ptr->dram_controller[bundle[i]->chan_id].rank[bundle[i]->rank_id].r_p_info.busy_last = sys_ptr->dram_controller[bundle[i]->chan_id].rank[bundle[i]->rank_id].r_p_info.busy_till;
      sys_ptr->dram_controller[bundle[i]->chan_id].rank[bundle[i]->rank_id].r_p_info.busy_till = completion_time;

      sys_ptr->dram_controller[bundle[i]->chan_id].rank[bundle[i]->rank_id].r_p_gblinfo.busy_last = sys_ptr->dram_controller[bundle[i]->chan_id].rank[bundle[i]->rank_id].r_p_gblinfo.busy_till;
      sys_ptr->dram_controller[bundle[i]->chan_id].rank[bundle[i]->rank_id].r_p_gblinfo.busy_till = completion_time;

      if ( sys_ptr->dram_controller[bundle[i]->chan_id].rank[bundle[i]->rank_id].r_p_info.busy_last > now) {
        sys_ptr->dram_controller[bundle[i]->chan_id].rank[bundle[i]->rank_id].r_p_info.amb_busy +=  sys_ptr->dram_controller[bundle[i]->chan_id].rank[bundle[i]->rank_id].r_p_info.busy_till -  sys_ptr->dram_controller[bundle[i]->chan_id].rank[bundle[i]->rank_id].r_p_info.busy_last;
        sys_ptr->dram_controller[bundle[i]->chan_id].rank[bundle[i]->rank_id].r_p_gblinfo.amb_busy +=  sys_ptr->dram_controller[bundle[i]->chan_id].rank[bundle[i]->rank_id].r_p_gblinfo.busy_till -  sys_ptr->dram_controller[bundle[i]->chan_id].rank[bundle[i]->rank_id].r_p_gblinfo.busy_last;
      }else {
        sys_ptr->dram_controller[bundle[i]->chan_id].rank[bundle[i]->rank_id].r_p_info.amb_busy +=  sys_ptr->dram_controller[bundle[i]->chan_id].rank[bundle[i]->rank_id].r_p_info.busy_till -  now;
        sys_ptr->dram_controller[bundle[i]->chan_id].rank[bundle[i]->rank_id].r_p_gblinfo.amb_busy +=  sys_ptr->dram_controller[bundle[i]->chan_id].rank[bundle[i]->rank_id].r_p_gblinfo.busy_till -  now;
      }
      //Take care of spill over cycles
      if (sys_ptr->dram_controller[bundle[i]->chan_id].rank[bundle[i]->rank_id].r_p_info.busy_till > dram_power_print_time) {
        sys_ptr->dram_controller[bundle[i]->chan_id].rank[bundle[i]->rank_id].r_p_info.amb_busy +=  - sys_ptr->dram_controller[bundle[i]->chan_id].rank[bundle[i]->rank_id].r_p_info.busy_till +  dram_power_print_time;
        sys_ptr->dram_controller[bundle[i]->chan_id].rank[bundle[i]->rank_id].r_p_info.amb_busy_spill_over +=  - dram_power_print_time + sys_ptr->dram_controller[bundle[i]->chan_id].rank[bundle[i]->rank_id].r_p_info.busy_till;

      }
    }
  }
  for (i=0;i<sys_ptr->config.rank_count;i++) {
    if (sys_ptr->dram_controller[chan_id].rank[i].r_p_info.busy_till < now) {
      sys_ptr->dram_controller[chan_id].rank[i].r_p_info.amb_data_pass_through += sys_ptr->config.t_bundle;
	  sys_ptr->dram_controller[chan_id].rank[i].r_p_gblinfo.amb_data_pass_through += sys_ptr->config.t_bundle;
	  //Take care of spill over cycles
	  if (now + sys_ptr->config.t_bundle > dram_power_print_time) {
		sys_ptr->dram_controller[chan_id].rank[i].r_p_info.amb_data_pass_through_spill_over +=  now + sys_ptr->config.t_bundle -  dram_power_print_time;
		sys_ptr->dram_controller[chan_id].rank[i].r_p_info.amb_data_pass_through +=  -now - sys_ptr->config.t_bundle +  dram_power_print_time;
	  }
	}
  }

}

tick_t get_command_completion_time(command_t *this_c) {
  return this_c->rank_done_time;
}
// Note that only r_p_info needs to make sure that power for a given interval
// is not over-calculated.
// r_p_gblinfo needs to make sure that power overall is calculated okay
//
void update_power_stats(tick_t now, command_t *this_c) {

  int chan_id = this_c->chan_id;
  int rank_id = this_c->rank_id;
  tick_t CAS_carry_cycle = 0;
  tick_t CAW_carry_cycle = 0;
  dram_system_configuration_t *pConfig = get_dram_system_config();

  if (this_c->command == CAS || this_c->command == CAS_WITH_DRIVE || this_c->command == CAS_AND_PRECHARGE) {
    dram_system.dram_controller[chan_id].rank[rank_id].r_p_info.current_CAS_cycle +=  pConfig->t_burst;
    dram_system.dram_controller[chan_id].rank[rank_id].r_p_gblinfo.sum_per_RD += pConfig->t_burst;
#ifdef DEBUG_POWER
    fprintf(stdout,"\t+++++rank[%d-%d] >>CAS_cycle[%llu] CAW_cycle[%llu] ",
        chan_id,rank_id,
        dram_system.dram_controller[chan_id].rank[rank_id].r_p_info.current_CAS_cycle,
        dram_system.dram_controller[chan_id].rank[rank_id].r_p_info.current_CAW_cycle);
    fprintf(stdout,"last_RAS[%llu]\n",
        (tick_t) dram_system.dram_controller[chan_id].rank[rank_id].r_p_info.last_RAS_time);
#endif
    //Ohm--stat for dram_access
    dram_system.dram_controller[chan_id].rank[rank_id].r_p_info.dram_access++;
    dram_system.dram_controller[chan_id].rank[rank_id].r_p_gblinfo.dram_access++;
    dram_system.dram_controller[chan_id].rank[rank_id].r_p_info.last_CAS_time = now;
    dram_system.dram_controller[chan_id].rank[rank_id].r_p_gblinfo.last_CAS_time = now;
  }
  if (this_c->command == CAS_WRITE || this_c->command == CAS_WRITE_AND_PRECHARGE) {
    dram_system.dram_controller[chan_id].rank[rank_id].r_p_info.current_CAW_cycle +=  pConfig->t_burst;
    dram_system.dram_controller[chan_id].rank[rank_id].r_p_gblinfo.sum_per_WR += pConfig->t_burst;
#ifdef DEBUG_POWER
    fprintf(stdout,"\tXXXXrank[%d-%d] >>CAS_cycle[%llu] CAW_cycle[%llu] ",
        chan_id,rank_id,
        dram_system.dram_controller[chan_id].rank[rank_id].r_p_info.current_CAS_cycle,
        dram_system.dram_controller[chan_id].rank[rank_id].r_p_info.current_CAW_cycle);
    fprintf(stdout,"last_RAS[%llu]\n",
        (tick_t) dram_system.dram_controller[chan_id].rank[rank_id].r_p_gblinfo.last_RAS_time);
#endif
    //Ohm--stat for dram_access
    dram_system.dram_controller[chan_id].rank[rank_id].r_p_info.dram_access++;
    dram_system.dram_controller[chan_id].rank[rank_id].r_p_gblinfo.dram_access++;
    dram_system.dram_controller[chan_id].rank[rank_id].r_p_info.last_CAW_time = now;
    dram_system.dram_controller[chan_id].rank[rank_id].r_p_gblinfo.last_CAW_time = now;
  }
  if (this_c->command == RAS) {
    /* Ohm--put code to track t_ACT HERE */
    dram_system.dram_controller[chan_id].rank[rank_id].r_p_info.RAS_count++;
    if (dram_system.dram_controller[chan_id].rank[rank_id].r_p_info.last_RAS_time == 0) {
      //At the beginning of the printing period
      dram_system.dram_controller[chan_id].rank[rank_id].r_p_info.last_RAS_time = now;
      dram_system.dram_controller[chan_id].rank[rank_id].r_p_info.RAS_count--;
    }
    dram_system.dram_controller[chan_id].rank[rank_id].r_p_info.RAS_sum +=
      (now - dram_system.dram_controller[chan_id].rank[rank_id].r_p_info.last_RAS_time);

    //Ohm: change how to calculate RD% and WR%
    if (dram_system.dram_controller[chan_id].rank[rank_id].r_p_info.current_CAS_cycle > 0) {
      if (dram_system.dram_controller[chan_id].rank[rank_id].r_p_info.current_CAS_cycle >
          now - dram_system.dram_controller[chan_id].rank[rank_id].r_p_info.last_RAS_time) {
        dram_system.dram_controller[chan_id].rank[rank_id].r_p_info.sum_per_RD +=
          now - dram_system.dram_controller[chan_id].rank[rank_id].r_p_info.last_RAS_time;
        CAS_carry_cycle = dram_system.dram_controller[chan_id].rank[rank_id].r_p_info.current_CAS_cycle -
          (now - dram_system.dram_controller[chan_id].rank[rank_id].r_p_info.last_RAS_time);
      }
      else
        dram_system.dram_controller[chan_id].rank[rank_id].r_p_info.sum_per_RD +=
          dram_system.dram_controller[chan_id].rank[rank_id].r_p_info.current_CAS_cycle;
    }
    if (dram_system.dram_controller[chan_id].rank[rank_id].r_p_info.current_CAW_cycle > 0) {
      if (dram_system.dram_controller[chan_id].rank[rank_id].r_p_info.current_CAW_cycle >
          now - dram_system.dram_controller[chan_id].rank[rank_id].r_p_info.last_RAS_time) {
        dram_system.dram_controller[chan_id].rank[rank_id].r_p_info.sum_per_WR +=
          now - dram_system.dram_controller[chan_id].rank[rank_id].r_p_info.last_RAS_time;
        CAW_carry_cycle = dram_system.dram_controller[chan_id].rank[rank_id].r_p_info.current_CAW_cycle -
          (now - dram_system.dram_controller[chan_id].rank[rank_id].r_p_info.last_RAS_time);
      }
      else
        dram_system.dram_controller[chan_id].rank[rank_id].r_p_info.sum_per_WR +=
          dram_system.dram_controller[chan_id].rank[rank_id].r_p_info.current_CAW_cycle;
    }
    dram_system.dram_controller[chan_id].rank[rank_id].r_p_info.current_CAS_cycle = CAS_carry_cycle;
    dram_system.dram_controller[chan_id].rank[rank_id].r_p_info.current_CAW_cycle = CAW_carry_cycle;
    dram_system.dram_controller[chan_id].rank[rank_id].r_p_info.last_RAS_time = now;
#ifdef DEBUG_POWER
    fprintf(stdout,"\t***RAS issued @rank[%d-%d] CAS_cycle[%llu] CAW_cycle[%llu] ",
        chan_id, rank_id,
        dram_system.dram_controller[chan_id].rank[rank_id].r_p_info.current_CAS_cycle,
        dram_system.dram_controller[chan_id].rank[rank_id].r_p_info.current_CAW_cycle);
    fprintf(stdout, "RAS_count[%llu] RAS_sum[%llu]",
        dram_system.dram_controller[chan_id].rank[rank_id].r_p_info.RAS_count,
        dram_system.dram_controller[chan_id].rank[rank_id].r_p_info.RAS_sum);
    fprintf(stdout, "sum_RD[%llu] sum_WR[%llu]\n",
        dram_system.dram_controller[chan_id].rank[rank_id].r_p_info.sum_per_RD,
        dram_system.dram_controller[chan_id].rank[rank_id].r_p_info.sum_per_WR);
#endif



    dram_system.dram_controller[chan_id].rank[rank_id].r_p_gblinfo.RAS_count++;
    if (dram_system.dram_controller[chan_id].rank[rank_id].r_p_gblinfo.last_RAS_time == 0) {
      //At the beginning
      dram_system.dram_controller[chan_id].rank[rank_id].r_p_gblinfo.last_RAS_time = now;
      dram_system.dram_controller[chan_id].rank[rank_id].r_p_gblinfo.RAS_count--;
    }

    dram_system.dram_controller[chan_id].rank[rank_id].r_p_gblinfo.RAS_sum +=
      (now - dram_system.dram_controller[chan_id].rank[rank_id].r_p_gblinfo.last_RAS_time);
    dram_system.dram_controller[chan_id].rank[rank_id].r_p_gblinfo.last_RAS_time = now;

  }
  // How does one do REFRESH_ONE_CHAN_ONE_RANK_ONE_BANK power
  // and REFRESH_ONE_CHAN_ALL_RANK_ONE_BANK
  if (this_c->command == REFRESH_ALL) {
       if (pConfig->refresh_policy == REFRESH_ONE_CHAN_ONE_RANK_ALL_BANK) {
        dram_system.dram_controller[chan_id].rank[rank_id].r_p_gblinfo.refresh_count++;
        dram_system.dram_controller[chan_id].rank[rank_id].r_p_info.refresh_count++;
       } else if (pConfig->refresh_policy == REFRESH_ONE_CHAN_ALL_RANK_ALL_BANK) {
         int i;
         for (i =0;i<pConfig->rank_count;i++) {
           dram_system.dram_controller[chan_id].rank[i].r_p_gblinfo.refresh_count++;
           dram_system.dram_controller[chan_id].rank[i].r_p_info.refresh_count++;
         }
       }
  }
}
