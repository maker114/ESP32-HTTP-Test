#ifndef MAIN_h
#define MAIN_h

void weather_update(void);
void Display_Mode1(void);
void Display_Mode2(void);
void Display_Mode3(void);
void Display_Mode4(void);
void Button_Scan(void);
void HTTP_LinkError_Handle(void);
void WIFI_Connect(void);
void Move_Cursor(int GoalValue, float *CurrentValue);
void NUM_Display(int num, int x, int y, float change[], int W, int H);

#endif