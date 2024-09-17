#include "func.h"
#include "c_ctl.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include "c_ctl.h"

int check_dim(binary_data *f1, binary_data *f2){
	size_t d1 = f1->info.tdef * f1->info.x.def * f1->info.y.def;
	size_t d2 = f2->info.tdef * f2->info.x.def * f2->info.y.def;

	return d1 == d2;
}