#ifndef __MACFIX_H_
#define __MACFIX_H_

struct details{
    uint32_t version; //版本号
    uint32_t custom_num; //客户编号
    uint32_t system_model; //系统类型
    uint32_t product_date;	//产品出厂日期
    uint32_t serial_num;		//产品流水号
    uint32_t check_emp_num;	//检测人员编号
};
#define STARTDAT 2010
extern char *user_usb_va;
/*extern void strpar(char *s,char *user_usb_va,char *user_va); */
extern char *user_va ;
extern char *s;

unsigned long strtol(const char *cp, char **endp,
				unsigned int base)
{
	unsigned long result = 0;
	unsigned long value;

	if (*cp == '0') {
		cp++;
		if ((*cp == 'x') && isxdigit(cp[1])) {
			base = 16;
			cp++;
		}

		if (!base)
			base = 8;
	}

	if (!base)
		base = 10;

	while (isxdigit(*cp) && (value = isdigit(*cp) ? *cp-'0' : (islower(*cp)
	    ? toupper(*cp) : *cp)-'A'+10) < base) {
		result = result*base + value;
		cp++;
	}

	if (endp){
		*endp = (char *)cp;
		//printf("endp is %p,*endp is %c\n",*endp,**endp);
	}
	return result;
}
unsigned long mysimple_strtoul(const char *cp, char **endp,
				unsigned int base)
{
	unsigned long result = 0;
	unsigned long value;

	if (*cp == '0') {
		cp++;
		if ((*cp == 'x') && isxdigit(cp[1])) {
			base = 16;
			cp++;
		}

		if (!base)
			base = 8;
	}

	if (!base)
		base = 10;

	while (isxdigit(*cp) && (value = isdigit(*cp) ? *cp-'0' : (islower(*cp)
	    ? toupper(*cp) : *cp)-'A'+10) < base) {
		result = result*base + value;
		cp++;
	}

	if (endp)
		*endp = (char *)cp;

	return result;
}		
void ubootstrtomac(unsigned int date,unsigned int swift_num,char *user_va,char *user_usb_va)
{
    int i=0;
    unsigned int year=0;
    unsigned int mon=0;
    unsigned int day=0;
    unsigned char mac[6]={0x00,0x90,0x4c,0x11,0x22,0x33};
    unsigned char mac1[6]={0x10,0x90,0x4c,0x44,0x55,0x66};
    char *mystr=NULL;
		
	//	user_va=(char *)kmalloc(18,GFP_KERNEL);
	//	user_usb_va=(char *)kmalloc(18,GFP_KERNEL);
		printk("user_va malloc  is %p,user_usb_va malloc is %p\n",user_va,user_usb_va);
		
    year=date/10000;
    mon=date%10000/100;
    day=date%10000%100;
			
    if(year !=0 &&mon !=0 &&day !=0)
    {
        mac[3]=mon+12*(year-STARTDAT);
        mac[4]=day;
        mac[5]=swift_num;

        mac1[3]=mac[3];
        mac1[4]=mac[4];
        mac1[5]=mac[5];
    }
    
    mystr=user_va;
    for(i=0;i<5;i++)
    {
        sprintf(mystr,"%02x:",mac[i]);
        mystr+=3;
    }
    sprintf(mystr,"%02x",mac[i]);

    user_va[strlen(user_va)]='\0';

    mystr=user_usb_va;
    for(i=0;i<5;i++)
    {
        sprintf(mystr,"%02x:",mac1[i]);
        mystr+=3;
    }
    sprintf(mystr,"%02x",mac1[i]);
    user_usb_va[strlen(user_usb_va)]='\0';
	printk("user_va after  is %s,user_usb_va after is %s\n",user_va,user_usb_va);
}
	/*add over */
int check_str_ok(struct details *info_fromloader)
 	{
		int i=0;
		int crc=0;
		char *p=(char *)info_fromloader;
		for(i=0;i<sizeof(struct details);i++)
		{
			crc+=p[i];
			}
			printk("crc_ok is %x\n",crc);
			return crc;
		}
		
 void strpar(char *s,char *user_usb_va,char *user_va)
 {
	 char s1[7][9]; // version cunstom_num	system_model product_date serial_num check_emp_num
	int i=0,j=0,t=0;
	int crc=0;
	struct details info;

	unsigned int date=0;
	unsigned int serial_num=0;
		for(i=0;i<7;i++)
	{
		for(j=0;j<8;j++)
		{
			s1[i][j]=s[t++];
		}
			s1[i][j]='\0';
	}
/*
 * uint32_t version; //版本号
    uint32_t custom_num; //客户编号
    uint32_t system_model; //系统类型
    uint32_t product_date;	//产品出厂日期
    uint32_t serial_num;		//产品流水号
    uint32_t check_emp_num;	//检测人员编号
 * */
	info.version =mysimple_strtoul(s1[0],NULL,16);
	info.custom_num=mysimple_strtoul(s1[1],NULL,16);
	info.system_model =mysimple_strtoul(s1[2],NULL,16);
	info.product_date =mysimple_strtoul(s1[3],NULL,16);
	info.serial_num=mysimple_strtoul(s1[4],NULL,16);
	info.check_emp_num =mysimple_strtoul(s1[5],NULL,16);
    if(mysimple_strtoul(s1[6],NULL,16) == check_str_ok(&info))
    {
		date = info.product_date;
		serial_num=info.serial_num;
	}
	else
	{
		date=mysimple_strtoul("1339f91",NULL,16);
		serial_num=mysimple_strtoul("1",NULL,16);
	}
printk("date is %u serial_num is %u \n",date,serial_num);
	ubootstrtomac(date,serial_num,user_va,user_usb_va);
	printk("user_va is %s ,user_usb_va is %s\n",user_va,user_usb_va);
	 
	 }

 
 
 #endif
