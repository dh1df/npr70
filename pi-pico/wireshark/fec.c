#include "fec.h"

 int FEC_decode(const unsigned char *RX_FIFO_data, int RX_FIFO_mask, int RX_FIFO_RD_point, unsigned char* data_out, int size_in, unsigned int* micro_BER) {
	int size_out; 
	int size_single_bloc;
	int size_single_bloc_pl1;
	unsigned char CRC_check_1;
	unsigned char CRC_check_2;
	unsigned char CRC_check_3;
	unsigned char CRC_check_4;
	unsigned char data_temp; 
	unsigned int nb_errors;
	int i; 
	size_single_bloc_pl1 = size_in / 4;
	size_single_bloc = size_single_bloc_pl1 - 1;
	
	CRC_check_1 = 0;
	CRC_check_2 = 0;
	CRC_check_3 = 0;
	CRC_check_4 = 0;
#if 0
        printf("%02x %d",RX_FIFO_data [(RX_FIFO_RD_point + size_in-1) & RX_FIFO_mask], size_single_bloc_pl1);
#endif
	for (i=0; i<size_single_bloc_pl1; i++) {
		CRC_check_1 = CRC_check_1 ^ RX_FIFO_data [(RX_FIFO_RD_point + i) & RX_FIFO_mask];
		CRC_check_2 = CRC_check_2 ^ RX_FIFO_data [(RX_FIFO_RD_point + size_single_bloc_pl1 + i) & RX_FIFO_mask];
		CRC_check_3 = CRC_check_3 ^ RX_FIFO_data [(RX_FIFO_RD_point + (2*size_single_bloc_pl1) + i) & RX_FIFO_mask];
#if 0
		printf(" %02x", RX_FIFO_data [(RX_FIFO_RD_point + (3*size_single_bloc_pl1) + i) & RX_FIFO_mask]);
#endif
		CRC_check_4 = CRC_check_4 ^ RX_FIFO_data [(RX_FIFO_RD_point + (3*size_single_bloc_pl1) + i) & RX_FIFO_mask];
	}
#if 0
	printf("\n");
#endif
	
	nb_errors = 0;
#if 0
        printf("check %d %d %d %d\n",CRC_check_1, CRC_check_2, CRC_check_3, CRC_check_4);
#endif
	if (CRC_check_1 != 0) { nb_errors++; }// printf("error 1  %X ;", CRC_check_1);}
	if (CRC_check_2 != 0) { nb_errors++; }// printf("error 2  %X ;", CRC_check_2);}
	if (CRC_check_3 != 0) { nb_errors++; }// printf("error 3  %X ;", CRC_check_3);}
	if (CRC_check_4 != 0) { nb_errors++; }// printf("error 4  %X ;", CRC_check_4);}
	//printf("\r\n");
	//if (nb_errors>0) {printf("ERR%i\r\n", nb_errors);}
	
	(*micro_BER) = nb_errors;
	
	if (nb_errors>1) {
		size_out = 0; //unrecoverable error
		//printf("unrecoverable\r\n");
	} else {
		// OK
		size_out = 3 * size_single_bloc; 
		// FIELD 1
		if (CRC_check_1==0) {//field 1 OK
			for (i=0; i<size_single_bloc; i++) {
				data_out[i] = RX_FIFO_data [(RX_FIFO_RD_point + i) & RX_FIFO_mask];
			}
		} else {// field 1 corrupted, reconstruction from field 2, 3 and 4
			for (i=0; i<size_single_bloc; i++) {
				data_temp = RX_FIFO_data [(RX_FIFO_RD_point + size_single_bloc_pl1 + i) & RX_FIFO_mask]; 
				data_temp = data_temp ^ RX_FIFO_data [(RX_FIFO_RD_point + (2*size_single_bloc_pl1) + i) & RX_FIFO_mask];
				data_temp = data_temp ^ RX_FIFO_data [(RX_FIFO_RD_point + (3*size_single_bloc_pl1) + i) & RX_FIFO_mask];
				data_out[i] = data_temp; 
			}
		}
		// FIELD 2
		if (CRC_check_2==0) {//field 2 OK
			for (i=0; i<size_single_bloc; i++) {
				data_out[size_single_bloc + i] = RX_FIFO_data [(RX_FIFO_RD_point + size_single_bloc_pl1 + i) & RX_FIFO_mask];
			}
		} else {// field 2 corrupted, reconstruction from field 1, 3 and 4
			for (i=0; i<size_single_bloc; i++) {
				data_temp = RX_FIFO_data [(RX_FIFO_RD_point + i) & RX_FIFO_mask]; //field 1
				data_temp = data_temp ^ RX_FIFO_data [(RX_FIFO_RD_point + (2*size_single_bloc_pl1) + i) & RX_FIFO_mask]; //field 3
				data_temp = data_temp ^ RX_FIFO_data [(RX_FIFO_RD_point + (3*size_single_bloc_pl1) + i) & RX_FIFO_mask]; //field 4
				data_out[size_single_bloc + i] = data_temp; 
			}
		}
		// FIELD 3
		if (CRC_check_3==0) {//field 3 OK
			for (i=0; i<size_single_bloc; i++) {
				data_out[(2*size_single_bloc) + i] = RX_FIFO_data[(RX_FIFO_RD_point + (2*size_single_bloc_pl1) + i) & RX_FIFO_mask];
			}
		} else {// field 3 corrupted, reconstruction from field 1, 2 and 4
			for (i=0; i<size_single_bloc; i++) {
				data_temp = RX_FIFO_data [(RX_FIFO_RD_point + i) & RX_FIFO_mask]; //field 1
				data_temp = data_temp ^ RX_FIFO_data [(RX_FIFO_RD_point + size_single_bloc_pl1 + i) & RX_FIFO_mask]; //field 2
				data_temp = data_temp ^ RX_FIFO_data [(RX_FIFO_RD_point + (3*size_single_bloc_pl1) + i) & RX_FIFO_mask]; //field 4
				data_out[(2*size_single_bloc) + i] = data_temp; 
			}
		}
	}
	RX_FIFO_RD_point = RX_FIFO_RD_point + size_in; 
	return size_out; 
}
