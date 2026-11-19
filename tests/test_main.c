#include "test_support.h"
#include <stdio.h>
#include <string.h>
int main(int argc, char **argv)
{
    if (argc >= 2 && !strcmp(argv[1],"error")) return test_error(argc,argv);
    if (argc != 2) return 2;
    test_begin(argv[1]);
    if (!strcmp(argv[1],"parts")) test_parts();
    else if (!strcmp(argv[1],"register")) test_registers();
    else if (!strcmp(argv[1],"decoder")) test_decoder();
    else if (!strcmp(argv[1],"mux")) test_mux();
    else if (!strcmp(argv[1],"rf")) test_rf();
    else if (!strcmp(argv[1],"im")) test_im();
    else if (!strcmp(argv[1],"dm")) test_dm();
    else if (!strcmp(argv[1],"student-a")) student_a();
    else if (!strcmp(argv[1],"student-b")) student_b();
    else { fputs("[ENV] unknown test group\n",stderr); return 2; }
    return test_finish();
}
