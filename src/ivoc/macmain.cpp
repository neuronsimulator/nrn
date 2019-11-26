extern "C" {
#pragma import on
int ivocmain(int, char**, char**, int);
#pragma import off
}

int main() {
	return ivocmain(0, 0, 0, 1);
}

