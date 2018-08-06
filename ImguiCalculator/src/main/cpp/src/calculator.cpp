//
// Created by cqhch on 8/5/2018.
//
#include "calculator.h"
#include <iostream>
#include <string>
#include <vector>
#include "logger.h"
#include <math.h>       /* sin */
#define PI 3.14159265

using namespace std;


int static nextOper(std::string& s)
{
    char ch;
    int n;

    for (int i=0; i<s.length(); ++i) {
        ch = s[i];

        if (ch == '+') {
            cin.putback(ch);
            n = 1;
            break;
        }
        else if (ch == '-') {
            cin.putback(ch);
            n = 2;
            break;
        }
        else if (ch == 'X') {
            cin.putback(ch);
            n = 3;
            break;
        }
        else if (ch == '/') {
            cin.putback(ch);
            n = 4;
            break;
        }

    }
    s.erase(0, 1);

    return n;
}

double static getInt(std::string& s)
{
    char ch;
    int i=0;
    std::string output = "0";
    char temp;

    if(s[0] == 's' || s[0] == 'c' || s[0] == 't'){


        temp = s[0];
        s.erase(0, 3);

    }

    for (i; i<s.length(); ++i)
    {

        ch = s[i];



        if((ch>='0' && ch<='9') || ch == '.'){


            std::ostringstream strs;
            strs << ch ;
            output += strs.str();

        } else {
            break;

        }
    }
    s.erase(0, i);
    double outDouble = std::stod (output);


    switch(temp){
        case 's':
            return sin ( outDouble * PI / 180.0 );
        case 'c':
            return cos ( outDouble * PI / 180.0 );
        case 't':
            return tan ( outDouble * PI / 180.0 );
        default:
            return outDouble;
    }

    return outDouble;
}


double static calculate(double num,double sum, int op)
{
    switch(op)
    {
        case 1: {sum+=num; break;}
        case 2: {sum-=num; break;}
        case 3: {sum*=num; break;}
        case 4: {sum/=num; break;}
        default: sum+=num;
    }

    return  sum;
}


double calculator::calEverything(std:: string strToCalculate){
    double sum = getInt(strToCalculate);
    while (strToCalculate != "") {

        Log(LOG_INFO) << "Got strToCalculate: " << strToCalculate;
        int opt = nextOper(strToCalculate);

        double temp = getInt(strToCalculate);

        sum = calculate(temp,sum,opt);

    }
    Log(LOG_INFO) << "Got sum: " << sum;

    return sum;

}
