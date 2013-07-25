#include <stdio.h>

int main() {
	int c;
	int i = 0;
	unsigned char hash;
	while ( (c = getchar() ) != EOF) {
		if (c == '\n') {
			printf(": %d\n", hash);
			hash = 0;
			i = 0;
		}
		else {
			if (i < 8) {
				hash += (c - 36);
				i++;
			}
		}
	}

	return 0;
}
