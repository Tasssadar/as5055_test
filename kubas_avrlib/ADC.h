//ATMega16
//10-bitovy
//125Khz = 9615sps
int ADC10Bit(uint8_t index)
{
	uint16_t value;
//	if (index <= 254)
//	{
		ADMUX = index;
//	}
//	else
//	{
//		return 0;
//	}
	ADCSRA = 199;
	while ((ADCSRA & (1 << ADSC)) == (1 << ADSC)) {}
	value = ADC;
	ADCSRA = (1 << ADIF);
	return value;
}
int ADC8Bit(uint8_t index)
{
	uint8_t value;
	if (index <= 7)
	{
		ADMUX = (32 + index);
	}
	else
	{
		return 0;
	}
	ADCSRA = 199;
	while ((ADCSRA & (1 << ADSC)) == (1 << ADSC)) {}
	value = ADCH;
	ADCSRA = (1 << ADIF);
	return value;
}
