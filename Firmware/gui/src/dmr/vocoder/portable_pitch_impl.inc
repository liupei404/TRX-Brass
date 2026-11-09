/* Internal fixed-point pitch tracking implementation. */


int vocoder_track_pitch_analysis_raw(short *argument_1,short *argument_2,intptr_t argument_3,intptr_t argument_4,uint32_t argument_5,
                int argument_6,uint32_t argument_7,uint16_t *argument_8)

{
  short short_value_1;
  uint16_t unsigned_value_2;
  int signed_value_3;
  uint32_t unsigned_value_4;
  short intermediate_high_1;
  short intermediate_high_2;
  uint32_t unsigned_value_5;
  short short_value_6;
  short short_value_7;
  uint32_t unsigned_value_8;
  short short_value_9;
  int signed_value_10;
  short short_value_11;
  int signed_value_12;
  uint32_t unsigned_value_13;
  short short_value_14;
  uint32_t unsigned_value_15;
  int signed_value_16;
  int signed_value_17;
  int signed_value_18;
  uint32_t unsigned_value_19;
  int signed_value_20;
  intptr_t local_pointer;
  bool condition_21;
  short work_7c;
  int work_78;
  short work_74;
  uint32_t work_70;
  uint32_t work_68;
  uint16_t work_58;
  uint16_t work_54;
  short work_52;
  short work_50;
  int work_4c;
  int work_48;
  int work_44;
  uint32_t work_40;
  uint32_t work_3c;
  int work_38;
  int work_34;
  int work_30;
  int work_2c [2];
  
  short_value_7 = argument_2[7];
  short_value_14 = *argument_2;
  argument_2[7] = argument_2[5];
  argument_2[4] = argument_2[2];
  argument_2[9] = short_value_7;
  argument_2[3] = argument_2[1];
  argument_2[8] = argument_2[6];
  argument_2[2] = short_value_14;
  if (argument_6 == 0) {
    argument_1[6] = 0;
    unsigned_value_8 = 0x1c;
    signed_value_3 = 8;
    signed_value_10 = 4;
  }
  else if (argument_1[6] < 3) {
pitch_branch_10:
    signed_value_3 = 8;
    unsigned_value_8 = 0x1c;
    signed_value_10 = 4;
  }
  else {
    if (argument_1[2] < 0x2400) {
      if (argument_1[2] < 0x2000) goto pitch_branch_10;
    }
    else if (0x23ff < short_value_14) {
      signed_value_3 = 0xc;
      unsigned_value_8 = 0x1a;
      signed_value_10 = 6;
      goto pitch_branch_01;
    }
    if (short_value_14 < 0x2000) {
      signed_value_3 = 8;
      unsigned_value_8 = 0x1c;
      signed_value_10 = 4;
    }
    else {
      signed_value_3 = 10;
      unsigned_value_8 = 0x1b;
      signed_value_10 = 5;
    }
  }
pitch_branch_01:
  signed_value_3 = PITCH_ARGMAX_I16(signed_value_3 + argument_3,unsigned_value_8);
  unsigned_value_4 = PITCH_REFINE_INDEX(signed_value_10 + signed_value_3,argument_3);
  unsigned_value_15 = unsigned_value_4;
  if ((int)unsigned_value_4 < 0) {
    unsigned_value_15 = ~unsigned_value_4;
  }
  short_value_14 = (short)unsigned_value_15;
  if (unsigned_value_4 != 0) {
    short_value_14 = (short)COUNT_LEADING_ZEROS(unsigned_value_15) + -1;
  }
  unsigned_value_4 = unsigned_value_4 << ((int)short_value_14 & 0xffU);
  work_4c = 0xf - short_value_14;
  unsigned_value_15 = unsigned_value_4 >> 0x10;
  work_58 = (uint16_t)(unsigned_value_4 >> 0x10);
  work_52 = 0;
  work_40 = 0;
  work_50 = 0;
  work_3c = 0;
  signed_value_3 = 1;
  work_54 = work_58;
  work_44 = work_4c;
  do {
    unsigned_value_19 = signed_value_3 * 0x10000 + 0x8000;
    unsigned_value_4 = unsigned_value_19;
    if ((int)unsigned_value_19 < 0) {
      unsigned_value_4 = ~unsigned_value_19;
    }
    short_value_14 = (short)unsigned_value_4;
    if (unsigned_value_19 != 0) {
      short_value_14 = (short)COUNT_LEADING_ZEROS(unsigned_value_4) + -1;
    }
    signed_value_20 = unsigned_value_19 << ((int)short_value_14 & 0xffU);
    unsigned_value_4 = 0xf - (int)short_value_14;
    signed_value_10 = work_4c + unsigned_value_4;
    unsigned_value_2 = (uint16_t)unsigned_value_15;
    signed_value_12 = (int)(short)((uint32_t)signed_value_20 >> 0x10) * (int)(short)unsigned_value_2 * 2;
    if (signed_value_10 < 0xf) {
      signed_value_12 = signed_value_12 >> (0xfU - signed_value_10 & 0xff);
    }
    else {
      signed_value_12 = signed_value_12 << (signed_value_10 - 0xfU & 0xff);
    }
    if (0x1f < signed_value_12 >> 0x10) break;
    unsigned_value_15 = -unsigned_value_4;
    if ((int)unsigned_value_15 < 0) {
      signed_value_16 = 0x40000000 >> (unsigned_value_4 & 0xff);
    }
    else {
      signed_value_16 = 0x40000000 << (unsigned_value_15 & 0xff);
    }
    signed_value_20 = signed_value_20 + signed_value_16 * -2;
    signed_value_16 = (int)(short)((uint32_t)signed_value_20 >> 0x10) * (int)(short)unsigned_value_2 * 2;
    if (signed_value_10 < 0xf) {
      signed_value_16 = signed_value_16 >> (0xfU - signed_value_10 & 0xff);
    }
    else {
      signed_value_16 = signed_value_16 << (signed_value_10 - 0xfU & 0xff);
    }
    signed_value_16 = (signed_value_16 >> 0x10) + 1;
    if ((int)unsigned_value_15 < 0) {
      signed_value_17 = 0x20000000 >> (unsigned_value_4 & 0xff);
    }
    else {
      signed_value_17 = 0x20000000 << (unsigned_value_15 & 0xff);
    }
    signed_value_20 = signed_value_20 + signed_value_17;
    signed_value_17 = (int)(short)((uint32_t)signed_value_20 >> 0x10) * (int)(short)unsigned_value_2 * 2;
    if (signed_value_10 < 0xf) {
      signed_value_17 = signed_value_17 >> (0xfU - signed_value_10 & 0xff);
    }
    else {
      signed_value_17 = signed_value_17 << (signed_value_10 - 0xfU & 0xff);
    }
    signed_value_17 = (signed_value_17 >> 0x10) + 1;
    if ((int)unsigned_value_15 < 0) {
      signed_value_18 = 0x40000000 >> (unsigned_value_4 & 0xff);
    }
    else {
      signed_value_18 = 0x40000000 << (unsigned_value_15 & 0xff);
    }
    signed_value_20 = (int)(short)((uint32_t)(signed_value_20 + signed_value_18) >> 0x10) * (int)(short)unsigned_value_2 * 2;
    if (signed_value_10 < 0xf) {
      signed_value_20 = signed_value_20 >> (0xfU - signed_value_10 & 0xff);
    }
    else {
      signed_value_20 = signed_value_20 << (signed_value_10 - 0xfU & 0xff);
    }
    signed_value_20 = ((signed_value_20 >> 0x10) - signed_value_17) + 1;
    local_pointer = argument_3 + signed_value_17 * 2;
    if (signed_value_3 != 1) {
      signed_value_18 = PITCH_ARGMAX_I16(local_pointer,signed_value_20);
      unsigned_value_4 = PITCH_REFINE_INDEX(signed_value_18 + signed_value_17,argument_3);
      unsigned_value_15 = unsigned_value_4;
      if ((int)unsigned_value_4 < 0) {
        unsigned_value_15 = ~unsigned_value_4;
      }
      short_value_14 = (short)unsigned_value_15;
      if (unsigned_value_4 != 0) {
        short_value_14 = (short)COUNT_LEADING_ZEROS(unsigned_value_15) + -1;
      }
      work_44 = 0xf - short_value_14;
      work_54 = (uint16_t)((unsigned_value_4 << ((int)short_value_14 & 0xffU)) >> 0x10);
    }
    signed_value_12 = PITCH_SUM_I16(argument_3 + signed_value_16 * 2,((signed_value_12 >> 0x10) - signed_value_16) + 1);
    unsigned_value_15 = PITCH_SUM_I16(local_pointer,signed_value_20);
    unsigned_value_4 = signed_value_12 - unsigned_value_15;
    if ((int)unsigned_value_4 < 1) {
pitch_branch_05:
      signed_value_12 = 0x16;
      work_38 = 7;
      short_value_14 = 0x6400;
      if (-1 < (int)unsigned_value_15) goto pitch_branch_04;
pitch_branch_06:
      unsigned_value_15 = 0;
    }
    else {
      unsigned_value_19 = unsigned_value_4;
      if ((int)unsigned_value_4 < 0) {
        unsigned_value_19 = ~unsigned_value_4;
      }
      short_value_14 = (short)unsigned_value_19;
      if (unsigned_value_4 != 0) {
        short_value_14 = (short)COUNT_LEADING_ZEROS(unsigned_value_19) + -1;
      }
      unsigned_value_19 = unsigned_value_15;
      if ((int)unsigned_value_15 < 0) {
        unsigned_value_19 = ~unsigned_value_15;
      }
      short_value_7 = (short)unsigned_value_19;
      if (unsigned_value_15 != 0) {
        short_value_7 = (short)COUNT_LEADING_ZEROS(unsigned_value_19) + -1;
      }
      unsigned_value_15 = unsigned_value_15 << ((int)short_value_7 & 0xffU);
      signed_value_10 = vocoder_normalized_divide
                         (unsigned_value_15,-(int)short_value_7,(int)(unsigned_value_4 << ((int)short_value_14 & 0xffU)) >> 0x10,
                          -(int)short_value_14,&work_38);
      if (7 < work_38) goto pitch_branch_05;
      if (work_38 == 7) {
        if (0x63ff < signed_value_10) {
          signed_value_10 = 0x6400;
        }
        signed_value_12 = 0x16;
      }
      else {
        signed_value_12 = work_38 + 0xf;
      }
      short_value_14 = (short)signed_value_10;
      if ((int)unsigned_value_15 < 0) goto pitch_branch_06;
pitch_branch_04:
      unsigned_value_15 = short_value_14 * signed_value_3 * 2;
    }
    unsigned_value_4 = unsigned_value_15;
    if ((int)unsigned_value_15 < 0) {
      unsigned_value_4 = ~unsigned_value_15;
    }
    short_value_14 = (short)unsigned_value_4;
    if (unsigned_value_15 != 0) {
      short_value_14 = (short)COUNT_LEADING_ZEROS(unsigned_value_4) + -1;
    }
    short_value_7 = (short)((unsigned_value_15 << ((int)short_value_14 & 0xffU)) >> 0x10);
    unsigned_value_4 = (int)(short)work_54 * (int)short_value_7 * 2;
    signed_value_12 = signed_value_12 - short_value_14;
    unsigned_value_15 = unsigned_value_4;
    if ((int)unsigned_value_4 < 0) {
      unsigned_value_15 = ~unsigned_value_4;
    }
    short_value_14 = (short)unsigned_value_15;
    if (unsigned_value_4 != 0) {
      short_value_14 = (short)COUNT_LEADING_ZEROS(unsigned_value_15) + -1;
    }
    work_48 = work_38;
    work_52 = vocoder_normalized_add((int)work_52,work_40,
                                   (int)(unsigned_value_4 << ((int)short_value_14 & 0xffU)) >> 0x10,
                                   (signed_value_12 + work_44) - (int)short_value_14,&work_40);
    unsigned_value_4 = short_value_7 * signed_value_3 * 2;
    unsigned_value_15 = unsigned_value_4;
    if ((int)unsigned_value_4 < 0) {
      unsigned_value_15 = ~unsigned_value_4;
    }
    short_value_14 = (short)unsigned_value_15;
    if (unsigned_value_4 != 0) {
      short_value_14 = (short)COUNT_LEADING_ZEROS(unsigned_value_15) + -1;
    }
    work_50 = vocoder_normalized_add((int)work_50,work_3c,
                                   (int)(unsigned_value_4 << ((int)short_value_14 & 0xffU)) >> 0x10,
                                   (signed_value_12 + 0xf) - (int)short_value_14,&work_3c);
    if (0 < work_50) {
      work_58 = vocoder_normalized_divide
                           ((int)work_52 << 0x10,work_40,(int)work_50,work_3c,&work_4c);
    }
    unsigned_value_15 = (uint32_t)work_58;
    signed_value_3 = signed_value_3 + 1;
    unsigned_value_2 = work_58;
  } while (signed_value_3 != 0x10);
  if (work_4c < 6) {
    signed_value_3 = ((int)(short)unsigned_value_2 << 0x10) >> (6U - work_4c & 0xff);
  }
  else {
    signed_value_3 = ((int)(short)unsigned_value_2 << 0x10) << (work_4c - 6U & 0xff);
  }
  signed_value_3 = signed_value_3 >> 0x10;
  short_value_14 = argument_1[6];
  if (signed_value_3 < 0x666) {
    signed_value_3 = 0x666;
  }
  if (0x3dff < signed_value_3) {
    signed_value_3 = 0x3e00;
  }
  if (short_value_14 < 1) {
    work_68 = (uint32_t)argument_2[2];
    unsigned_value_15 = (uint32_t)argument_1[2];
    work_70 = unsigned_value_15;
  }
  else {
    unsigned_value_15 = (uint32_t)argument_2[2];
    work_70 = (int)argument_1[2];
    work_68 = unsigned_value_15;
  }
  unsigned_value_2 = argument_1[4];
  short_value_7 = argument_1[5];
  short_value_1 = argument_2[7];
  work_7c = (short)signed_value_3;
  if (signed_value_3 < 0x199b) {
    unsigned_value_19 = PITCH_SUM_INTERVALS(argument_3,signed_value_3,6,0x40);
    unsigned_value_4 = unsigned_value_19;
    if ((int)unsigned_value_19 < 0) {
      unsigned_value_4 = ~unsigned_value_19;
    }
    short_value_11 = (short)unsigned_value_4;
    if (unsigned_value_19 != 0) {
      short_value_11 = (short)COUNT_LEADING_ZEROS(unsigned_value_4) + -1;
    }
    unsigned_value_13 = (uint32_t)short_value_11;
    work_78 = (int)(unsigned_value_19 << (unsigned_value_13 & 0xff)) >> 0x10;
    unsigned_value_4 = -unsigned_value_13;
    signed_value_10 = PITCH_TAIL_COMPLEMENT(work_78,unsigned_value_4,argument_3,signed_value_3,6);
    if (0xccc < signed_value_3) goto pitch_branch_12;
pitch_branch_07:
    short_value_11 = (short)signed_value_10;
pitch_branch_02:
    argument_2[5] = short_value_11;
  }
  else {
    signed_value_20 = (int)work_7c;
    signed_value_10 = PITCH_SUM_BANDS(&work_38,argument_4,0x10000,signed_value_20,work_2c);
    signed_value_12 = work_2c[0] + signed_value_20 * -0x18 >> 0x10;
    work_34 = 0;
    signed_value_16 = PITCH_SUM_I32(argument_4 + signed_value_12 * 4,0x80 - signed_value_12,&work_34);
    intermediate_high_1 = (short)((uint32_t)signed_value_16 >> 0x10);
    if (work_38 - work_34 < 0) {
      signed_value_10 = (signed_value_10 << 0x10) >> (work_34 - work_38 & 0xffU);
    }
    else {
      signed_value_10 = (signed_value_10 << 0x10) << (work_38 - work_34 & 0xffU);
    }
    if (0 < signed_value_10 + intermediate_high_1 * -0xe8ba) {
      signed_value_10 = signed_value_20 * 0x20 + 0x10000 >> 0x10;
      work_30 = 0;
      signed_value_16 = PITCH_SUM_I32(argument_4 + signed_value_10 * 4,0x80 - signed_value_10,&work_30);
      intermediate_high_2 = (short)((uint32_t)signed_value_16 >> 0x10);
      if (work_34 - work_30 < 0) {
        signed_value_10 = ((int)intermediate_high_1 << 0x10) >> (work_30 - work_34 & 0xffU);
      }
      else {
        signed_value_10 = ((int)intermediate_high_1 << 0x10) << (work_34 - work_30 & 0xffU);
      }
      if (signed_value_10 + intermediate_high_2 * -0x1998 < 1) goto pitch_branch_11;
      unsigned_value_8 = PITCH_SUM_BANDS(&work_38,argument_4,signed_value_20,signed_value_20,work_2c);
      signed_value_10 = PITCH_NORMALIZED_COMPLEMENT(unsigned_value_8,work_38,(int)intermediate_high_2,work_30);
      short_value_11 = (short)signed_value_10;
      if (signed_value_10 < 0) goto pitch_branch_11;
      goto pitch_branch_02;
    }
pitch_branch_11:
    unsigned_value_19 = PITCH_SUM_INTERVALS(argument_3,signed_value_3,6,0x40);
    unsigned_value_4 = unsigned_value_19;
    if ((int)unsigned_value_19 < 0) {
      unsigned_value_4 = ~unsigned_value_19;
    }
    short_value_11 = (short)unsigned_value_4;
    if (unsigned_value_19 != 0) {
      short_value_11 = (short)COUNT_LEADING_ZEROS(unsigned_value_4) + -1;
    }
    unsigned_value_13 = (uint32_t)short_value_11;
    unsigned_value_4 = -unsigned_value_13;
    work_78 = (int)(unsigned_value_19 << (unsigned_value_13 & 0xff)) >> 0x10;
    signed_value_10 = PITCH_TAIL_COMPLEMENT(work_78,unsigned_value_4,argument_3,signed_value_3,6);
pitch_branch_12:
    unsigned_value_5 = PITCH_SUM_INTERVALS(argument_3,signed_value_3,5,0x40);
    unsigned_value_19 = unsigned_value_5;
    if ((int)unsigned_value_5 < 0) {
      unsigned_value_19 = ~unsigned_value_5;
    }
    short_value_11 = (short)unsigned_value_19;
    if (unsigned_value_5 != 0) {
      short_value_11 = (short)COUNT_LEADING_ZEROS(unsigned_value_19) + -1;
    }
    unsigned_value_19 = (uint32_t)short_value_11;
    short_value_11 = (short)((unsigned_value_5 << (unsigned_value_19 & 0xff)) >> 0x10);
    signed_value_12 = (int)short_value_11;
    if ((int)(1 - unsigned_value_13) < (int)-unsigned_value_19) {
      unsigned_value_4 = ~unsigned_value_4 - unsigned_value_19;
      signed_value_20 = work_78 << 0xf;
pitch_branch_13:
      signed_value_16 = (int)short_value_11;
      signed_value_17 = signed_value_16 * 0xe8ba;
      if ((int)unsigned_value_4 < 1) {
        unsigned_value_13 = signed_value_17 >> (-unsigned_value_4 & 0xff);
      }
      else {
        unsigned_value_13 = signed_value_17 << (unsigned_value_4 & 0xff);
        condition_21 = signed_value_17 - ((int)unsigned_value_13 >> (unsigned_value_4 & 0xff)) != 0;
        if (condition_21) {
          unsigned_value_13 = 0x7fffffff;
        }
        if (condition_21) {
          unsigned_value_13 = unsigned_value_13 ^ signed_value_16 / 0x8ccd + (signed_value_16 >> 0x1f);
        }
      }
      if (signed_value_20 < (int)unsigned_value_13) {
        if (((short_value_1 < signed_value_10) || (short_value_1 < 0x199a)) && (argument_6 != 0)) {
          signed_value_16 = (signed_value_3 << 0x10) >> 1;
          signed_value_17 = signed_value_16 + unsigned_value_15 * -0x10000;
          unsigned_value_13 = unsigned_value_15;
pitch_joined_branch:
          if (signed_value_17 < 0) {
            if (signed_value_17 == -0x80000000) {
              signed_value_17 = 0x7fffffff;
            }
            else {
              signed_value_17 = -signed_value_17;
            }
          }
          signed_value_18 = signed_value_3 * 0x10000 + unsigned_value_13 * -0x10000;
          if (signed_value_18 < 0) {
            if (signed_value_18 == -0x80000000) {
              signed_value_18 = 0x7fffffff;
            }
            else {
              signed_value_18 = -signed_value_18;
            }
          }
          if (signed_value_17 < signed_value_18) goto pitch_branch_14;
        }
        else if (short_value_7 < 0xccd) {
          signed_value_16 = (int)short_value_11;
          signed_value_17 = signed_value_16 * 0xb6dc;
          if ((int)unsigned_value_4 < 1) {
            unsigned_value_13 = signed_value_17 >> (-unsigned_value_4 & 0xff);
          }
          else {
            unsigned_value_13 = signed_value_17 << (unsigned_value_4 & 0xff);
            condition_21 = signed_value_17 - ((int)unsigned_value_13 >> (unsigned_value_4 & 0xff)) != 0;
            if (condition_21) {
              unsigned_value_13 = 0x7fffffff;
            }
            if (condition_21) {
              unsigned_value_13 = unsigned_value_13 ^ signed_value_16 / 0xb333 + (signed_value_16 >> 0x1f);
            }
          }
          if (signed_value_20 < (int)unsigned_value_13) {
            signed_value_16 = signed_value_3 << 0xf;
            goto pitch_branch_14;
          }
        }
        else {
          signed_value_16 = signed_value_12 * 0x10000;
          if ((int)unsigned_value_4 < 1) {
            unsigned_value_13 = signed_value_16 >> (-unsigned_value_4 & 0xff);
          }
          else {
            unsigned_value_13 = signed_value_16 << (unsigned_value_4 & 0xff);
            condition_21 = signed_value_16 - ((int)unsigned_value_13 >> (unsigned_value_4 & 0xff)) != 0;
            if (condition_21) {
              unsigned_value_13 = 0x7fffffff;
            }
            if (condition_21) {
              unsigned_value_13 = unsigned_value_13 ^ signed_value_12 / 0x8001 + (signed_value_12 >> 0x1f);
            }
          }
          if ((short_value_7 * -0x2666 + 0x599a0000 >> 0x10) * work_78 * 2 < (int)unsigned_value_13) {
            signed_value_16 = (signed_value_3 << 0x10) >> 1;
            signed_value_17 = signed_value_16 + (uint32_t)unsigned_value_2 * -0x10000;
            unsigned_value_13 = (uint32_t)unsigned_value_2;
            goto pitch_joined_branch;
          }
        }
      }
      if (2 < short_value_14) {
        signed_value_16 = signed_value_12 * 0x10000;
        if ((int)unsigned_value_4 < 1) {
          unsigned_value_13 = signed_value_16 >> (-unsigned_value_4 & 0xff);
        }
        else {
          unsigned_value_13 = signed_value_16 << (unsigned_value_4 & 0xff);
          condition_21 = signed_value_16 - ((int)unsigned_value_13 >> (unsigned_value_4 & 0xff)) != 0;
          if (condition_21) {
            unsigned_value_13 = 0x7fffffff;
          }
          if (condition_21) {
            unsigned_value_13 = unsigned_value_13 ^ signed_value_12 / 0x8001 + (signed_value_12 >> 0x1f);
          }
        }
        if (signed_value_20 <= (int)unsigned_value_13) {
          signed_value_16 = signed_value_3 * 0x10000 >> 1;
          signed_value_20 = signed_value_16 + unsigned_value_15 * -0x10000;
          if (signed_value_20 < 0) {
            if (signed_value_20 == -0x80000000) {
              signed_value_20 = 0x7fffffff;
            }
            else {
              signed_value_20 = -signed_value_20;
            }
          }
          signed_value_17 = signed_value_3 * 0x10000 + unsigned_value_15 * -0x10000;
          if (signed_value_17 < 0) {
            if (signed_value_17 == -0x80000000) {
              signed_value_17 = 0x7fffffff;
            }
            else {
              signed_value_17 = -signed_value_17;
            }
          }
          if (signed_value_20 < signed_value_17) goto pitch_branch_14;
        }
      }
      goto pitch_branch_07;
    }
    unsigned_value_4 = ~unsigned_value_4 - unsigned_value_19;
    signed_value_20 = short_value_11 * 0x8a60;
    if ((int)unsigned_value_4 < 1) {
      unsigned_value_13 = signed_value_20 >> (-unsigned_value_4 & 0xff);
    }
    else {
      unsigned_value_13 = signed_value_20 << (unsigned_value_4 & 0xff);
      condition_21 = signed_value_20 - ((int)unsigned_value_13 >> (unsigned_value_4 & 0xff)) != 0;
      if (condition_21) {
        unsigned_value_13 = 0x7fffffff;
      }
      if (condition_21) {
        unsigned_value_13 = unsigned_value_13 ^ signed_value_20 >> 0x1f;
      }
    }
    signed_value_20 = work_78 * 0x8000;
    if ((((int)unsigned_value_13 <= signed_value_20) ||
        (((signed_value_16 = signed_value_3 * 0x10000, 2 < short_value_14 && (-1 < (int)(signed_value_16 + work_68 * -0xe666))) &&
         ((int)(signed_value_3 * 0xe8ba + work_68 * -0x10000) < 0x10000)))) ||
       ((-1 < (int)(signed_value_16 + work_70 * -0xd99a) &&
        ((int)(signed_value_3 * 0xde9c + work_70 * -0x10000) < 0x10000)))) goto pitch_branch_13;
    signed_value_16 = signed_value_16 >> 1;
pitch_branch_14:
    short_value_14 = PITCH_TAIL_COMPLEMENT(signed_value_12,-unsigned_value_19,argument_3,signed_value_3,5);
    work_7c = (short)((uint32_t)(signed_value_16 + 0x8000) >> 0x10);
    argument_2[5] = short_value_14;
  }
  *argument_2 = work_7c;
  *argument_8 = 0;
  short_value_1 = argument_2[5];
  short_value_11 = argument_2[2];
  short_value_14 = argument_2[7];
  short_value_7 = *argument_2;
  work_74 = argument_1[2];
  unsigned_value_15 = (uint32_t)(uint16_t)argument_1[4];
  signed_value_3 = (int)short_value_1;
  signed_value_12 = (int)short_value_7;
  signed_value_10 = (int)argument_1[3];
  if (signed_value_3 < 0xccd) {
    work_74 = (short)((uint32_t)((short_value_7 * 0x2666 + work_74 * 0x599a) * 2) >> 0x10);
  }
  unsigned_value_19 = signed_value_12 * 0x10000 + short_value_11 * -0x10000;
  unsigned_value_4 = unsigned_value_19;
  if ((int)unsigned_value_19 < 0) {
    if (unsigned_value_19 != 0x80000000) {
      unsigned_value_4 = -unsigned_value_19;
      goto pitch_branch_03;
    }
    signed_value_20 = short_value_7 * 0x4ccd >> 1;
    short_value_9 = 0;
    unsigned_value_4 = 0x7fffffff;
  }
  else {
pitch_branch_03:
    signed_value_20 = short_value_7 * 0x4ccd >> 1;
    if ((int)unsigned_value_4 < signed_value_20) {
      short_value_9 = argument_1[6] + 1;
    }
    else {
      short_value_9 = 0;
    }
  }
  if (((signed_value_3 < 0x199a) && (short_value_14 < 0x199a)) && ((int)unsigned_value_4 < signed_value_20)) {
    unsigned_value_15 = argument_1[3] * 0xfae2 + (short_value_7 * 0x51ec >> 4);
    unsigned_value_4 = unsigned_value_19;
    if ((int)unsigned_value_19 < 0) {
      unsigned_value_4 = ~unsigned_value_19;
    }
    short_value_6 = (short)unsigned_value_4;
    if (unsigned_value_19 != 0) {
      short_value_6 = (short)COUNT_LEADING_ZEROS(unsigned_value_4) + -1;
    }
    signed_value_16 = unsigned_value_19 << ((int)short_value_6 & 0xffU);
    unsigned_value_4 = -(int)short_value_6;
    signed_value_10 = (int)(unsigned_value_15 + 0x8000) >> 0x10;
    short_value_6 = 0x7fff;
    signed_value_20 = signed_value_12;
  }
  else {
    unsigned_value_19 = *argument_1 * 0xcccc;
    unsigned_value_4 = unsigned_value_19;
    if ((int)unsigned_value_19 < 0) {
      unsigned_value_4 = ~unsigned_value_19;
    }
    short_value_6 = (short)unsigned_value_4;
    if (unsigned_value_19 != 0) {
      short_value_6 = (short)COUNT_LEADING_ZEROS(unsigned_value_4) + -1;
    }
    signed_value_16 = unsigned_value_19 << ((int)short_value_6 & 0xffU);
    unsigned_value_4 = (int)argument_1[1] - (int)short_value_6;
    short_value_6 = (short)((uint32_t)(argument_1[5] * 0xe666) >> 0x10);
    signed_value_20 = (int)argument_1[4];
  }
  signed_value_17 = (int)short_value_14;
  condition_21 = (int)unsigned_value_4 < 0;
  if (condition_21) {
    unsigned_value_15 = -unsigned_value_4;
  }
  signed_value_18 = (signed_value_16 >> 0x10) * 0xe666;
  if (condition_21) {
    signed_value_18 = signed_value_18 >> (unsigned_value_15 & 0xff);
  }
  if (!condition_21) {
    signed_value_18 = signed_value_18 << (unsigned_value_4 & 0xff);
  }
  signed_value_20 = pitch_saturate_i32((int64_t)signed_value_20 * 0xe666);
  DISCARD_SATURATION_STATUS(signed_value_20,0x20);
  signed_value_20 = pitch_saturate_i32((int64_t)signed_value_20 + signed_value_18);
  DISCARD_SATURATION_STATUS(signed_value_20,0x20);
  signed_value_20 = pitch_saturate_i32(
      (int64_t)signed_value_20 + ((int64_t)signed_value_10 * 0x6666 >> 2));
  DISCARD_SATURATION_STATUS(signed_value_20,0x20);
  signed_value_20 = signed_value_20 >> 0x10;
  argument_1[1] = (short)unsigned_value_4;
  if (signed_value_20 < 0x666) {
    signed_value_20 = 0x666;
  }
  if (0x3dff < signed_value_20) {
    signed_value_20 = 0x3e00;
  }
  argument_1[4] = (short)signed_value_20;
  *argument_1 = (short)((uint32_t)signed_value_16 >> 0x10);
  argument_1[3] = (short)signed_value_10;
  argument_1[5] = short_value_6;
  argument_1[2] = work_74;
  argument_1[6] = short_value_9;
  if ((0x3333 < signed_value_3) && (signed_value_17 < 0x3333)) {
    argument_2[1] = short_value_11;
    argument_2[6] = short_value_14;
    return (int)work_7c;
  }
  if (signed_value_17 < 0x3334) {
    if (0x199a < signed_value_17) {
pitch_branch_08:
      if (signed_value_3 < signed_value_17 >> 1) goto pitch_branch_09;
    }
    argument_2[1] = (short)(signed_value_12 + short_value_11 >> 1);
    argument_2[6] = (short)(signed_value_3 + signed_value_17 >> 1);
  }
  else {
    if (0x3332 < signed_value_3) goto pitch_branch_08;
pitch_branch_09:
    argument_2[1] = short_value_7;
    argument_2[6] = short_value_1;
  }
  return (int)work_7c;
}
