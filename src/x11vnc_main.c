/*
 * x11vnc_main.c - Main entry point wrapper
 *
 * This simply calls the original x11vnc main function
 */

/* Declare the original main function */
int x11vnc_main_legacy(int argc, char* argv[]);

int main(int argc, char* argv[]) {
    return x11vnc_main_legacy(argc, argv);
}