#ifndef TimeController_h
#define TimeController_h
void initTimeController(bool useNtp);
bool updateTimeController();
unsigned long getCurrentUtcTime();
char* getCurrentUtcTimeAsJsonField();
#endif
