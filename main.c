//------KHAI BAO THU VIEN CHO CHUONG TRINH---------------
#include <REGX52.H>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "..\Lib\LCD_1602.h"
#include "..\Lib\delay.h"
#include "..\Lib\PWM.h"
#include "..\Lib\beep.h"
#include "..\Lib\UART_Mode1.h"
#include "..\Lib\DHT_11.h"
/******CHU THICH CHAN TREN 8051**********
	P0 // du lieu hien thi LCD
	P1 //nhap du lieu cho keypad 4X4
	
	sbit pwm = P2^0;
	sbit IR = P2^2;
	sbit buzzer = P2^5;
	sbit RS = P2^6;
	sbit RW = P2^5;
	sbit EN = P2^7;
	
	sbit DHT11 = P2^3;
	sbit setup_auto = P3^3;
	
*/
//----KHAI BAO CHAN TREN 8051----
sbit sel_servo = P2^4;						
sbit IR = P2^2;							//cam bien IR phat hien vat can
sbit setup_auto = P3^3;						//nut dieu chinh che do auto
sbit sta_auto = P3^4;
//----PHAN KHAI BAO BIEN TOAN CUC----
char password[6]= "123456";					//bien luu gia tri password
char num;
unsigned char byte_data;						
int set_auto = 0;							//mac dinh tat che do tu dong mo cua
int I_RH,D_RH,I_Temp,D_Temp,CheckSum,check = 0;	//khai bao cac bien luu gia tri cam bien DHT11
//----KHAI BAO NGUYEN MAU HAM----
unsigned char Scan_Keypad();
void Open_Door(unsigned char row, unsigned char col, char *s);
void Close_Door(unsigned char row, unsigned char col, char *s);
void Uart_Command();
//----CHUONG TRINH CHINH----
void main(){
	char dat[20];
	char nhap[6] ="";					//khai bao bien luu mat khau nhap vao tu keypad
	unsigned char KT = 0,i = 0;						//khai bien dem so luong chu so da nhap vao
									//--------
	sel_servo = 0;
	EA = 1;								//cho phep ngat toan cuc
	EX1 = 1;								//cho phep ngat ngoai 0
	IT1 = 1;								//ngat ngoai 0 o che do canh xuong
	ES = 1;								//cho phep ngat truyen thong
									//--------
	LCD_Init();							//khoi tao LCD
	pwm_init(20000);						//khoi tao servo chu ky 20ms
	set_duty(7);	 						//khoi tao do rong xung 2.5% la 0 do, 7.5% la 90 do, 11,9% la 180 do
	pwm_start();    						//cho phep servo hoat dong
	
	LCD_String_xy(0,0,"PASSWORD or SCAN");		//mac dinh se hien thi yeu cau nhap password hoac quet the
	LCD_String_xy(1,4,"______");
	while(1){
	uart_init();										//khoi tao uart
	if(set_auto==0){
													//quet ban phim kiem tra gia tri phim nhan
		if(Scan_Keypad()){
			nhap[i] = num;
			LCD_String_xy(1,i+4,"");
			LCD_Char('*');
			if(nhap[i] == password[i]) KT++;
			i++;										//dem so luong chu so da nhap
			if(i == 6){
				
				if(KT == 6){
					Open_Door(1,0,"PASSWORD DUNG");		//mo cua va thong bao ra LCD
					delay_ms(2000);					//cho it nhat 2s
					while(IR==0);						// cho khi cam bien van phat hien vat can
					Close_Door(1,0,"    ______       ");
				}
				else{
					LCD_String_xy(1,0,"             ");
					LCD_String_xy(1,0,"SAI PASSWORD");		//thong bao ra LCD
					delay_ms(2000);					//cho 2s
					LCD_String_xy(1,0,"    ______     ");
					delay_ms(2000);
				}
				i = 0;								//khoi tao lai so luong chu so ve 0
				KT = 0;
				}
			}
		}
											//khi o che do tu dong va cam bien phat hien vat can thi tu dong mo cua
		else if(IR==0 && set_auto == 1){
			set_duty(7.5);						//do rong xung 7.5% servo mo cua
			delay_ms(3000);
			while(IR==0);
			set_duty(2.5);		
		}
										//thuc hien lenh gui tu ESP32
		Uart_Command();
									//Neu co nhan yeu cau tu ESP32 se bat dau do nhiet do, do am
		if(check == 1){
			Request();
			Response();
			I_RH = Receive_data();
			D_RH = Receive_data();
			I_Temp = Receive_data();
			D_Temp = Receive_data();
			CheckSum = Receive_data();
			if(I_RH + D_RH + I_Temp + D_Temp != CheckSum) ;
			else{
				sprintf(dat,"{\"Hum\":%d.%d,\"Temp\":%d.%d}",I_RH,D_RH,I_Temp,D_Temp);
				}
		}
		if(check==1){
			uart_init();
			uart_write_text(dat);			//gui chuoi du lieu den ESP32
			check=0;
		}
		if(set_auto == 1) set_duty(2.5);
		else set_duty(7);
	}
}
									//-----PHAN KHAI BAO NGAT
void ngat_uart() interrupt 4{
	if(RI){ 
		byte_data = uart_read();	
	}
}
void ngat_button() interrupt 2{				// chuyen doi bat/tat che do tu dong mo mua
	beep_delay();
	delay_ms(100);
	if(P3_3==0){
		set_auto = !set_auto;
		if(set_auto==1){
			sel_servo = 1;
			uart_write_text('A');
			LCD_Command(0x01);
			LCD_String_xy(0,2,"CUA TU DONG");
		}
		else{
			sel_servo = 0;
			uart_write_text('N');
			LCD_Command(0x01);
			LCD_String_xy(0,0,"PASSWORD or SCAN");
			LCD_String_xy(1,4,"______");
		}
	}
	while(P3_3==0);
}
									//----CAC HAM PHU TRONG CHUONG TRINH----
void Uart_Command(){
	if(byte_data=='S' && set_auto==0){			//U la Unlock
			Open_Door(1,0,"The hop le     ");
			delay_ms(3000);
			Close_Door(1,0,"    ______     ");	
	}
	else if(byte_data == 'F' && set_auto==0){				//F la Failed tuc khong the mo cua
			LCD_String_xy(1,0,"The khong hop le");
			delay_ms(3000);
			Close_Door(1,0,"    ______     ");	
	}	
	if(byte_data=='U' && set_auto==0){			//U la Unlock
			Open_Door(1,0,"Cua mo       ");
			delay_ms(3000);
			Close_Door(1,0,"    ______     ");	
	}
	else if(byte_data == 'A'){				//A la Auto
			set_auto = 1 ;
			sel_servo = 1;
			LCD_Command(0x01);
			set_duty(2.5);
			LCD_String_xy(0,2,"CUA TU DONG");
	}	
	else if(byte_data == 'N') {				//N la Not Auto
			set_auto = 0;
			sel_servo = 0;
			LCD_Command(0x01);
			LCD_String_xy(0,0,"PASSWORD or SCAN");
			LCD_String_xy(1,4,"______");
	}
	else if(byte_data == 'D'){				//esp muon nhan nhiet do
		check = 1;
	}
	else if(byte_data == 'C'){
		char nhap[6]; unsigned char i = 0;
		EA = 0;
		LCD_Command(0x01);
		LCD_String_xy(0,0,"ENTER NEW PASS");
		while(i<6){
			if(Scan_Keypad()){
				password[i] = num;
				LCD_String_xy(1,i+4,"*");
				i++;
			}
		}
		if(set_auto==1){
			LCD_Command(0x01);
			LCD_String_xy(0,2,"CUA TU DONG");
		}
		else{
			LCD_Command(0x01);
			LCD_String_xy(0,0,"PASSWORD or SCAN");
			LCD_String_xy(1,4,"______");
		}
		EA=1;
	}
	byte_data = 0;
}
								//-----HAM DONG/MO CUA VA HIEN THI TREN LCD-----
void Open_Door(unsigned char row, unsigned char col, char *s){
	LCD_String_xy(row,col,s);
	set_duty(2);						//do rong xung 7.5% servo mo cua
}

void Close_Door(unsigned char row, unsigned char col, char *s){
	LCD_String_xy(row,col,s);
	set_duty(7);						//do rong xung 2.5% servo dong
}
								//-----PHAN CODE HOAT DONG CUA KEYPAD 4X4-----
unsigned char Scan_Keypad(){
	ES=0;
	P1 = 0xFF;
	P1_5 = 0;
		if(P1_3 == 0){beep_delay();num = '7'; while(P1_3 == 0); return 1;} 
		else if(P1_2 == 0) {beep_delay();num = '8';while(P1_2 == 0); return 1;}
		  else if(P1_1 == 0) {beep_delay();num = '9';while(P1_1 == 0);return 1; }
			else if(P1_0 == 0) {beep_delay();num = 'C';while(P1_0 == 0);return 1;}
	P1_5 = 1;
	
	P1_6 = 0;
		if(P1_3 == 0) {beep_delay();num = '4';while(P1_3 == 0);return 1; }
			else if(P1_2 == 0) {beep_delay();num = '5';while(P1_2 == 0); return 1;}
				else if(P1_1 == 0) {beep_delay();num = '6';while(P1_1 == 0);return 1; }
					else if(P1_0 == 0) {beep_delay();num = 'B';while(P1_0 == 0);return 1;}
	P1_6 = 1;
	
	P1_7 = 0;
		if(P1_3 == 0) {beep_delay();num = '1';while(P1_3 == 0);return 1;}
			else if(P1_2 == 0) {beep_delay();num = '2';while(P1_2 == 0);return 1;}
				else if(P1_1 == 0) {beep_delay();num = '3';while(P1_1 == 0);return 1;}
					else if(P1_0 == 0){beep_delay();num = 'A'; while(P1_0 == 0);return 1;}
	P1_7 = 1;
	
	P1_4 = 0;
		if(P1_3 == 0) {beep_delay();num = 'F';while(P1_3 == 0);return 1; }
			else if(P1_2 == 0) {beep_delay();num = '0';while(P1_2 == 0); return 1;}
				else if(P1_1 == 0) {beep_delay();num = 'E'; while(P1_1 == 0);return 1;}
					else if(P1_0 == 0) {beep_delay();num ='D'; while(P1_0 == 0);return 1; }
	P1_4 = 1;
	ES = 1;
	return 0;
}