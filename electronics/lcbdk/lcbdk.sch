EESchema Schematic File Version 4
EELAYER 26 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L MCU_Module:Arduino_Nano_v3.x A?
U 1 1 60ECED0E
P 6500 4500
F 0 "A?" H 6500 3414 50  0000 C CNN
F 1 "Arduino_Nano_v3.x" H 6500 3323 50  0000 C CNN
F 2 "Module:Arduino_Nano" H 6650 3550 50  0001 L CNN
F 3 "http://www.mouser.com/pdfdocs/Gravitech_Arduino_Nano3_0.pdf" H 6500 3500 50  0001 C CNN
	1    6500 4500
	1    0    0    -1  
$EndComp
$Comp
L Comparator:LM311 U?
U 1 1 60ECEDFC
P 3300 2900
F 0 "U?" H 3641 2946 50  0000 L CNN
F 1 "LM311" H 3641 2855 50  0000 L CNN
F 2 "" H 3300 2900 50  0001 C CNN
F 3 "http://www.ti.com/lit/ds/symlink/lm311.pdf" H 3300 2900 50  0001 C CNN
	1    3300 2900
	1    0    0    -1  
$EndComp
$Comp
L Display_Character:HY1602E DS?
U 1 1 60ECEEB6
P 9850 3950
F 0 "DS?" H 9850 4928 50  0000 C CNN
F 1 "HY1602E" H 9850 4837 50  0000 C CNN
F 2 "Display:HY1602E" H 9850 3050 50  0001 C CIN
F 3 "http://www.icbank.com/data/ICBShop/board/HY1602E.pdf" H 10050 4050 50  0001 C CNN
	1    9850 3950
	1    0    0    -1  
$EndComp
$EndSCHEMATC
