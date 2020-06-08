void kernel_main() {
	char *screen_buffer = (void*)0xB8000;
	char msg[] = "Hello, World!";
	unsigned int i = 24 * 80;
	screen_buffer[i * 2] = *msg;
	int j = 0;
	while (msg[j] != '\0') {
		screen_buffer[i * 2] = *(msg + j);
		j++;
		i++;
	}	
}