#if defined _Save_included
  #endinput
#endif

#include <amxmodx>

new where[]	= "addons/amxmodx/point"

/* Savev */

public SaveData(const file[], const key[], const data[])
{
	new s
	new c[1024], where_[128], arg1[64], txt[500], len;
	
	formatex(where_, 127, "%s/%s", where, file);
	
	formatex(c, 1023, "^"%s^" ^"%s^"", key, data);
	
	if(!dir_exists(where))
		mkdir(where);	
		
	if(!file_exists(where_))
		write_file(where_, "", 0);
		
	for(new i; read_file(where_, i, txt, 499, len);i++)
	{
		parse(txt, arg1, 63);
		
		if(equal(key, arg1, strlen(arg1)))
		{
			write_file(where_, c, i); 
			s++
		}	
		else	
			continue;
	}
	if(!s)
		write_file(where_, c, -1);
	s = 0
}

/* Load */

public LoadData(const file[], const key[], data[], len)
{
	new txt[100], len, where_[128];
	new arg1[40], arg2[40];
	
	formatex(where_, 127, "%s/%s", where, file) 
	
	if(!file_exists(where_))
		return PLUGIN_CONTINUE
	for(new i; read_file(where_, i, txt, 99, len);i++)
	{
		parse(txt, arg1, 39, arg2, 39);
		
		if(equal(key, arg1, strlen(arg1)))
			copy(data, len, arg2); 
		else	
			continue;
	}
	return PLUGIN_CONTINUE
}

/* Delete */

public DeleteData(const file[], const key[])
{

	new txt[1101], len, where_[128];
	new arg1[64], arg2[1024];
	
	formatex(where_, 127, "%s/%s", where, file) 
	
	if(!file_exists(where_))
		return PLUGIN_CONTINUE
	for(new i; read_file(where_, i, txt, 1100, len);i++)
	{
		parse(txt, arg1, 63, arg2, 1023);
		
		if(equal(key, arg1, strlen(arg1)))
			write_file(where_, "", i);
		else	
			continue;
	}
	return PLUGIN_CONTINUE
}

