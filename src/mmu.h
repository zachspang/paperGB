class MMU {
public:
	//Read byte and components other than the cpu 1 t-cycle
	uint8_t read(uint16_t addr);

	//Write byte and components other than the cpu 1 t-cycle
	void write(uint16_t addr, uint8_t byte);
};