// SuperHero MySQL Include File
// Version 1.18e

//----------------------------------------------------------------------------------------------
// Abstract Methods that must be implemented by a save superhero include file...
//
// writeMemoryTable()
// loadXP( id, key[35] )
// cleanXP(clearAll = false)
//----------------------------------------------------------------------------------------------
/*

//Use this to create the tables

CREATE TABLE `sh_savexp` (
	`SH_KEY` varchar(34) binary NOT NULL default '',
	`PLAYER_NAME` varchar(32) binary NOT NULL default '',
	`LAST_PLAY_DATE` timestamp(14) NOT NULL,
	`XP` int(10) unsigned NOT NULL default '0',
	`HUDHELP` tinyint(1) unsigned NOT NULL default '1',
	`SKILL_COUNT` int(3) unsigned NOT NULL default '0',
	PRIMARY KEY  (`SH_KEY`)
) TYPE=MyISAM COMMENT='SUPERHERO XP Saving Table';

CREATE TABLE `sh_saveskills` (
	`SH_KEY` varchar(34) binary NOT NULL default '',
	`SKILL_NUMBER` int(10) unsigned NOT NULL default '0',
	`HERO_NAME` varchar(25) NOT NULL default '',
	PRIMARY KEY  (`SH_KEY`,`SKILL_NUMBER`)
) TYPE=MyISAM COMMENT='SUPERHERO Skill Saving Table';

//Upgrade from prior to 1.17.5
ALTER TABLE `sh_savexp` ADD `HUDHELP` TINYINT( 1 ) UNSIGNED DEFAULT '1' NOT NULL AFTER `XP` ;

*/

#if defined _shsave_included || !defined SHCORE
  #endinput
#endif
#define _shsave_included

//Comment this out to use old syntax for MySQL 3.23
#define NEW_MYSQL

#if defined AMXX_VERSION
  #include <dbi>
  new Sql:gMySQL = SQL_FAILED
#else
  #include <mysql>
  new gMySQL = 0
#endif

// Global to this save class
new gVaultFile[128]
new gPersistent, gPersistentTemp = 0
new bool:CreateTables = true

//----------------------------------------------------------------------------------------------
public saving_init()
{
	register_cvar("sh_mysql_host","")
	register_cvar("sh_mysql_user","")
	register_cvar("sh_mysql_pass","")
	register_cvar("sh_mysql_db","")
	register_cvar("sh_mysql_persistent","0")

	register_concmd("amx_shvaulttosql","adminCopyVault", ADMIN_RCON,"- Copies vault XP to MySQL, should only be done once.")

	#if defined AMXX_VERSION
		get_localinfo("amxx_vault",gVaultFile,127)
	#else
		#if defined AMX_NEW
			copy(gVaultFile,127,"addons/amx/config/vault.ini")
		#else
			copy(gVaultFile,127,"addons/amx/vault.ini")
		#endif
	#endif
}
//----------------------------------------------------------------------------------------------
public adminCopyVault(id,level,cid)
{
	if (!cmd_access(id,level,cid,1)) return

	mySQLConnect()

	#if defined AMXX_VERSION
		if ( gMySQL <= SQL_FAILED ) return
	#else
		if ( gMySQL < 1 ) return
	#endif

	//Turn Persistant on for this function
	gPersistentTemp = true

	// Open up the vault file - read line by line
	if ( !file_exists(gVaultFile) ) return

	console_print(id,"Please wait while the XP data is copied")

	// Read through the file looking for shinfo entries
	new parm[2]
	parm[0] = id
	parm[1] = 0
	adminCopyVHelper(parm)

	//Rest of this is done in the helper function
}
//----------------------------------------------------------------------------------------------
public adminCopyVHelper(parm[])
{
	//Get data from parm
	new id = parm[0]
	new nextLine = parm[1]

	// Read through the file looking for shinfo entries
	new lengthRead
	new data[512]

	while ((nextLine = read_file(gVaultFile,nextLine,data,511,lengthRead)) != 0) {
		if ( lengthRead > 0 && containi(data, "shinfo") == 0 ) {
			parseXP(gMemoryTableSize - 1, data)
			if (nextLine % 200 == 0) {
				parm[1] = nextLine
				console_print(id,"Delaying a few seconds before copying the next set of entries")
				set_task(3.0, "adminCopyVHelper",0,parm,2)
				return
			}
		}
	}

	//If it makes it this far it must be done copying

	// OK now load the XP for any players playing
	new key[35]

	for ( new x = 1; x <= SH_MAXSLOTS; x++ ) {
		if (!is_user_connected(x)) continue
		getSaveKey(x, key)
		loadXP(x, key)
	}
	console_print(id,"Saved XP has been copied from the vault to the MySQL Database Successfully")

	//Set Persistent back how it was
	gPersistentTemp = false

	//Closes the connection if set not to be persistent
	close_mysql()
}
//----------------------------------------------------------------------------------------------
public parseXP(id, const data[] )
{
	// "name" epochtime XP HUDHELP "SKILL1" "SKILL2" ...
	new heroIndex
	new Left[35], Right[1501]

	copy(Right, 1500, data)

	//Get the key out
	strbrkqt(Right, Left, 34, Right, 1500)
	replace(Left, 34, "shinfo.", "" )
	replace_all(Left,34,"`","\`")
	replace_all(Left,34,"'","\'")
	copy( gMemoryTableKeys[id], 34, Left)

	//Get the name out
	strbrkqt(Right, Left, 34, Right, 1500)
	copy( gMemoryTableNames[id], 31, Left)

	//Get the epochtime out
	strbrkqt(Right, Left, 34, Right, 1500)

	//Get the XP out and save it
	strbrkqt(Right, Left, 34, Right, 1500)
	gMemoryTableXP[id] = str_to_num(Left)

	//Get the HUD Status out
	strbrkqt(Right, Left, 34, Right, 1500)
	gMemoryTableFlags[id] = str_to_num(Left)

	//Now load in all the skills
	new powerCount = 0
	new MaxPowers = min(get_cvar_num("sh_maxpowers"), SH_MAXLEVELS)

	while ( strlen(Right) > 0 && powerCount < MaxPowers ) {
		strbrkqt(Right, Left, 34, Right, 1500)
		heroIndex = findHero(Left)
		if ( heroIndex >= 0 )  {
			gMemoryTablePowers[id][0] = ++powerCount
			gMemoryTablePowers[id][powerCount] = heroIndex
		}
	}

	// Now save it to the SQL database
	writeData(id)
}
//----------------------------------------------------------------------------------------------
public writeMemoryTable()
{
	if ( !get_cvar_num("sh_savexp") ) return

	debugMessage("Writing XP Data to SQL Database",0,3)

	//Turn Persistant on for this function
	gPersistentTemp = true

	// Write Memory Table to File
	for (new x = 1; x < gMemoryTableCount; x++) {
		if ( strlen(gMemoryTableKeys[x]) > 0 ) {
			writeData(x)
		}
		// Can even clear the MemoryKey - if a player is disconnected no reason to save again and again...
		copy(gMemoryTableKeys[x], 34, "")
	}

	// No need to keep disconnected player around...
	gMemoryTableCount = 33

	//Set Persistent back how it was
	gPersistentTemp = false

	//Closes the connection if set not to be persistent
	close_mysql()
}
//----------------------------------------------------------------------------------------------
//
//AMXX SQL API Code
//
#if defined AMXX_VERSION
//----------------------------------------------------------------------------------------------
public mySQLConnect()
{
	gPersistent = get_cvar_num("sh_mysql_persistent")

	if ( gMySQL >= SQL_OK ) {
		if (!gPersistent && !gPersistentTemp) close_mysql()
		else return
	}

	new host[64],user[32],pass[32],db[32],error[128]

	get_cvar_string("sh_mysql_host",host,63)
	get_cvar_string("sh_mysql_user",user,31)
	get_cvar_string("sh_mysql_pass",pass,31)
	get_cvar_string("sh_mysql_db",db,31)

	if ( !strlen(host) && !strlen(user) && !strlen(db) ) {
		get_cvar_string("amx_mysql_host",host,63)
		get_cvar_string("amx_mysql_user",user,31)
		get_cvar_string("amx_mysql_pass",pass,31)
		get_cvar_string("amx_mysql_db",db,31)
	}

	gMySQL = dbi_connect(host,user,pass,db,error,127)
	if (gMySQL <= SQL_FAILED) {
		format(debugt,255,"MySQL connect error: '%s' (%s,%s,%s)",error,host,user,db)
		debugMessage(debugt,0,0)
		return
	}
	//Only try to create the tables once
	else if (CreateTables) {
		new sql[512]
		new Result:retval

		copy(sql, 511, "CREATE TABLE IF NOT EXISTS `sh_savexp` ( `SH_KEY` varchar(34) binary NOT NULL default '', `PLAYER_NAME` varchar(32) binary NOT NULL default '', `LAST_PLAY_DATE` timestamp(14) NOT NULL, `XP` int(10) unsigned NOT NULL default '0', `HUDHELP` tinyint(1) unsigned NOT NULL default '1', `SKILL_COUNT` int(3) unsigned NOT NULL default '0', PRIMARY KEY  (`SH_KEY`) ) TYPE=MyISAM COMMENT='SUPERHERO XP Saving Table'")
		retval = dbi_query(gMySQL,sql)
		if (retval <= RESULT_FAILED) {
			dbi_error(gMySQL,error,127)
			format(debugt,255,"Error Making Tables: '%s' - '%s'", error, sql)
			debugMessage(debugt,0,0)
			return
		}

		copy(sql, 511, "CREATE TABLE IF NOT EXISTS `sh_saveskills` ( `SH_KEY` varchar(34) binary NOT NULL default '', `SKILL_NUMBER` int(10) unsigned NOT NULL default '0', `HERO_NAME` varchar(25) NOT NULL default '', PRIMARY KEY  (`SH_KEY`,`SKILL_NUMBER`) ) TYPE=MyISAM COMMENT='SUPERHERO Skill Saving Table'")
		retval = dbi_query(gMySQL,sql)
		if (retval <= RESULT_FAILED) {
			dbi_error(gMySQL,error,127)
			format(debugt,255,"Error Making Tables: '%s' - '%s'", error, sql)
			debugMessage(debugt,0,0)
			return
		}

		CreateTables = false
	}
}
//----------------------------------------------------------------------------------------------
// Flushes data in memory table position x to database...
public writeData(x)
{
	mySQLConnect()
	if ( gMySQL <= SQL_FAILED ) return

	debugMessage("Trying to save XP data to MySQL database", x, 8)

	new error[128],sql[512]
	new Result:retval, Result:retvalins

	new bkqtname[65]
	copy(bkqtname,64,gMemoryTableNames[x])
	replace_all(bkqtname,64,"`","\`")
	replace_all(bkqtname,64,"'","\'")

	//Thanks to HC for help with writing more efficient queries
	//Check if this user has an entry already, if not make one
	format(sql, 511, "SELECT * FROM `sh_savexp` WHERE `SH_KEY` = '%s'", gMemoryTableKeys[x] )
	retval = dbi_query(gMySQL,sql)

	if (retval <= RESULT_FAILED) {
		dbi_error(gMySQL,error,127)
		format(debugt,255,"Error Querying MySQL DB for %s: '%s' - '%s'", gMemoryTableKeys[x], error, sql)
		debugMessage(debugt,0,0)
		return
	}
	else if (retval == RESULT_NONE || !dbi_nextrow(retval)) {
		format(sql, 511, "INSERT INTO `sh_savexp` (SH_KEY) VALUES ('%s')", gMemoryTableKeys[x] )
		retvalins = dbi_query(gMySQL,sql)
		if (retvalins <= RESULT_FAILED) {
			dbi_error(gMySQL,error,127)
			format(debugt,255,"Error Writing MySQL XP for %s: '%s' - '%s'", gMemoryTableKeys[x], error, sql)
			debugMessage(debugt,0,0)
			return
		}
	}

	if (retval >= RESULT_OK) dbi_free_result(retval)

	//Update users entry with current data
	format(sql, 511, "UPDATE `sh_savexp` SET `PLAYER_NAME`='%s', `LAST_PLAY_DATE`=SYSDATE(), `XP`='%d', `HUDHELP`='%d', `SKILL_COUNT`='%d' WHERE SH_KEY='%s'",bkqtname,gMemoryTableXP[x],gMemoryTableFlags[x],gMemoryTablePowers[x][0],gMemoryTableKeys[x] )
	retval = dbi_query(gMySQL,sql)
	if (retval <= RESULT_FAILED) {
		dbi_error(gMySQL,error,127)
		format(debugt,255,"Error Writing MySQL XP for %s: '%s' - '%s'", gMemoryTableKeys[x], error, sql)
		debugMessage(debugt,0,0)
		return
	}

	if (!is_user_connected(x) || gChangedHeroes[x]) {

		//Remove all saved powers for this user
		format(sql,511, "DELETE FROM `sh_saveskills` WHERE SH_KEY='%s'",gMemoryTableKeys[x])
		retval = dbi_query(gMySQL,sql)
		if (retval <= RESULT_FAILED) {
			dbi_error(gMySQL,error,127)
			format(debugt,255,"Error Deleting MySQL Skills for %s: '%s' - '%s'", gMemoryTableKeys[x], error, sql)
			debugMessage(debugt,0,0)
			return
		}

		// Saving by SuperHeroName since the hero order in the plugin.ini can change...
		for (new p = 1; p <= gMemoryTablePowers[x][0]; p++) {
			new heroIndex = gMemoryTablePowers[x][p]
			format(sql, 511, "INSERT INTO `sh_saveskills` VALUES ('%s','%d','%s')", gMemoryTableKeys[x],p,gSuperHeros[heroIndex][hero] )
			retval = dbi_query(gMySQL,sql)
			if (retval <= RESULT_FAILED) {
				dbi_error(gMySQL,error,127)
				format(debugt,255,"Error Writing MySQL XP for %s: '%s' - '%s'", gMemoryTableKeys[x], error, sql)
				debugMessage(debugt,0,0)
				return
			}
		}

		if (x > 0 && x <= SH_MAXSLOTS) {
			gChangedHeroes[x] = false
		}
	}

	//Closes the connection if set not to be persistent
	close_mysql()
}
//----------------------------------------------------------------------------------------------
public loadXP( id, key[] )
{
	mySQLConnect()
	if ( gMySQL <= SQL_FAILED ) return false

	debugMessage("Trying to load XP data from MySQL database", id, 8)

	new sql[512], error[128]
	new xp[32], skillCount[10], userFlags[64]
	new Result:retvalxp, Result:retvalskill
	new skills = 0

	format(sql, 511, "SELECT `XP`, `HUDHELP`, `SKILL_COUNT` FROM `sh_savexp` WHERE `SH_KEY` = '%s'", key)
	retvalxp = dbi_query(gMySQL,sql)

	if (retvalxp <= RESULT_FAILED) {
		dbi_error(gMySQL,error,127)
		format(debugt,255,"Error Loading MySQL XP for %s: '%s' - '%s'", key, error, sql)
		debugMessage(debugt,0,0)
		return false
	}
	else if (retvalxp == RESULT_NONE || !dbi_nextrow(retvalxp)) {
		dbi_error(gMySQL,error,127)
		format(debugt,127,"No Saved XP Loaded for %s: '%s'", key, error)
		debugMessage(debugt,id,1)

		if (retvalxp >= RESULT_OK) dbi_free_result(retvalxp)

		//Closes the connection if set not to be persistent
		close_mysql()

		return true
	}

	dbi_field(retvalxp, 1, xp, 31)
	gPlayerXP[id] = str_to_num(xp)
	gPlayerLevel[id] = getLevel(id)
	setLevel(id, gPlayerLevel[id] )

	dbi_field(retvalxp, 2, userFlags, 63)
	gPlayerFlags[id] = str_to_num(userFlags)

	dbi_field(retvalxp, 3, skillCount, 9)

	if (retvalxp >= RESULT_OK) dbi_free_result(retvalxp)

	format(sql, 511, "SELECT `HERO_NAME` FROM `sh_saveskills` WHERE `SH_KEY` = '%s' AND `SKILL_NUMBER` <= '%s' ORDER BY `SKILL_NUMBER` ASC", key, skillCount )
	retvalskill = dbi_query(gMySQL,sql)

	if (retvalskill <= RESULT_FAILED) {
		dbi_error(gMySQL,error,127)
		format(debugt,255,"Error Loading MySQL XP for %s: '%s' - '%s'", key, error, sql)
		debugMessage(debugt,0,0)
		return false
	}

	gPlayerPowers[id][0] = 0

	if (retvalskill != RESULT_NONE) {
		while (dbi_nextrow(retvalskill)) {
			new heroName[25]
			dbi_field(retvalskill, 1, heroName, 24)
			new heroIndex = getHeroIndex(heroName)
			if ( heroIndex >= 0 && getHeroLevel(heroIndex) <= gPlayerLevel[id] ) {
				gPlayerPowers[id][0]= ++skills
				gPlayerPowers[id][skills] = heroIndex
				initHero(id, heroIndex)
			}
		}
	}

	if (retvalskill >= RESULT_OK) dbi_free_result(retvalskill)

	updateMemoryTable(id)

	//Closes the connection if set not to be persistent
	close_mysql()

	return true
}
//----------------------------------------------------------------------------------------------
public cleanXP( bool:clearAll )
{
	if ( !clearAll && (!get_cvar_num("sh_savexp") || get_cvar_num("sh_xpsavedays") < 0)) return

	new error[128], sql[255]
	new Result:retval

	mySQLConnect()
	if ( gMySQL <= SQL_FAILED ) return

	if (clearAll) {
		format(sql, 254, "TRUNCATE TABLE `sh_saveskills`")
		retval = dbi_query(gMySQL,sql)
		if (retval <= RESULT_FAILED) {
			dbi_error(gMySQL,error,127)
			format(debugt,255,"Couldn't Erase XP: '%s' - '%s'", error, sql)
			debugMessage(debugt,0,0)
			return
		}

		format(sql, 254, "TRUNCATE TABLE `sh_savexp`")
		retval = dbi_query(gMySQL,sql)
		if (retval <= RESULT_FAILED) {
			dbi_error(gMySQL,error,127)
			format(debugt,255,"Couldn't Erase XP: '%s' - '%s'", error, sql)
			debugMessage(debugt,0,0)
			return
		}
	}
	else {

		#if defined NEW_MYSQL

		format(sql, 254, "DELETE FROM `sh_saveskills` USING `sh_saveskills`, `sh_savexp` WHERE sh_savexp.`SH_KEY` = sh_saveskills.`SH_KEY` AND sh_savexp.`LAST_PLAY_DATE` < (SYSDATE() - INTERVAL '%d' DAY)", get_cvar_num("sh_xpsavedays") )
		retval = dbi_query(gMySQL,sql)
		if (retval <= RESULT_FAILED) {
			dbi_error(gMySQL,error,127)
			format(debugt,255,"Couldn't Clean Powers: '%s' - '%s'", error, sql)
			debugMessage(debugt,0,0)
			return
		}

		format(sql, 254, "DELETE FROM `sh_savexp` WHERE `LAST_PLAY_DATE` < (SYSDATE() - INTERVAL '%d' DAY)", get_cvar_num("sh_xpsavedays") )
		retval = dbi_query(gMySQL,sql)
		if (retval <= RESULT_FAILED) {
			dbi_error(gMySQL,error,127)
			format(debugt,255,"Couldn't Clean XP: '%s' - '%s'", error, sql)
			debugMessage(debugt,0,0)
			return
		}

		#else

		format(sql, 254, "SELECT `SH_KEY` FROM `sh_savexp` WHERE `LAST_PLAY_DATE` < (SYSDATE() - INTERVAL '%d' DAY)", get_cvar_num("sh_xpsavedays") )
		retval = dbi_query(gMySQL,sql)
		if (retval <= RESULT_FAILED) {
			dbi_error(gMySQL,error,127)
			format(debugt,255,"Couldn't SELECT to Clean XP: '%s' - '%s'", error, sql)
			debugMessage(debugt,0,0)
			return
		}

		new Result:retvaldel

		if (retval != RESULT_NONE) {
			while (dbi_nextrow(retval)) {
				new sh_key[65]
				dbi_field(retval, 1, sh_key, 64)
				if (strlen(sh_key) <=0) return

				//Need to escape in case we get some bot names
				replace_all(sh_key,64,"`","\`")
				replace_all(sh_key,64,"'","\'")

				format(sql,511, "DELETE FROM `sh_savexp` WHERE SH_KEY='%s'",sh_key)
				retvaldel = dbi_query(gMySQL,sql)
				if (retvaldel <= RESULT_FAILED) {
					dbi_error(gMySQL,error,127)
					format(debugt,255,"Couldn't Clean XP: '%s' - '%s'", error, sql)
					debugMessage(debugt,0,0)
					return
				}

				format(sql,511, "DELETE FROM `sh_saveskills` WHERE SH_KEY='%s'",sh_key)
				retvaldel = dbi_query(gMySQL,sql)
				if (retvaldel <= RESULT_FAILED) {
					dbi_error(gMySQL,error,127)
					format(debugt,255,"Couldn't Clean Powers: '%s' - '%s'", error, sql)
					debugMessage(debugt,0,0)
					return
				}
			}
		}
		if (retval >= RESULT_OK) dbi_free_result(retval)

		#endif
	}

	//Closes the connection if set not to be persistent
	close_mysql()
}
//----------------------------------------------------------------------------------------------
public close_mysql()
{
	if (gMySQL <= SQL_FAILED || gPersistent || gPersistentTemp) return
	dbi_close(gMySQL)
}
//----------------------------------------------------------------------------------------------
public saving_end()
{
	if ( gMySQL <= SQL_FAILED ) return
	dbi_close(gMySQL)
}
//----------------------------------------------------------------------------------------------
//
//AMX SQL API Code
//
#else
//----------------------------------------------------------------------------------------------
public mySQLConnect()
{
	gPersistent = get_cvar_num("sh_mysql_persistent")

	if (gMySQL > 0 ) {
		if (!gPersistent && !gPersistentTemp) close_mysql()
		else return
	}

	new host[64],user[32],pass[32],db[32],error[128]

	get_cvar_string("sh_mysql_host",host,63)
	get_cvar_string("sh_mysql_user",user,31)
	get_cvar_string("sh_mysql_pass",pass,31)
	get_cvar_string("sh_mysql_db",db,31)

	if ( !strlen(host) && !strlen(user) && !strlen(db) ) {
		get_cvar_string("amx_mysql_host",host,63)
		get_cvar_string("amx_mysql_user",user,31)
		get_cvar_string("amx_mysql_pass",pass,31)
		get_cvar_string("amx_mysql_db",db,31)
	}

	gMySQL = mysql_connect(host,user,pass,db,error,127)
	if (gMySQL < 1) {
		format(debugt,255,"MySQL connect error: '%s' (%s,%s,%s) - %d",error,host,user,db,gMySQL)
		debugMessage(debugt,0,0)
		return
	}
	//Only try to create the tables once
	else if (CreateTables) {
		new sql[512]

		copy(sql, 511, "CREATE TABLE IF NOT EXISTS `sh_savexp` ( `SH_KEY` varchar(34) binary NOT NULL default '', `PLAYER_NAME` varchar(32) binary NOT NULL default '', `LAST_PLAY_DATE` timestamp(14) NOT NULL, `XP` int(10) unsigned NOT NULL default '0', `HUDHELP` tinyint(1) unsigned NOT NULL default '1', `SKILL_COUNT` int(3) unsigned NOT NULL default '0', PRIMARY KEY  (`SH_KEY`) ) TYPE=MyISAM COMMENT='SUPERHERO XP Saving Table'")
		if (mysql_query(gMySQL,sql) < 1) {
			mysql_error(gMySQL,error,127)
			format(debugt,255,"Error Making Tables: '%s' - '%s'", error, sql)
			debugMessage(debugt,0,0)
			return
		}

		copy(sql, 511, "CREATE TABLE IF NOT EXISTS `sh_saveskills` ( `SH_KEY` varchar(34) binary NOT NULL default '', `SKILL_NUMBER` int(10) unsigned NOT NULL default '0', `HERO_NAME` varchar(25) NOT NULL default '', PRIMARY KEY  (`SH_KEY`,`SKILL_NUMBER`) ) TYPE=MyISAM COMMENT='SUPERHERO Skill Saving Table'")
		if (mysql_query(gMySQL,sql) < 1) {
			mysql_error(gMySQL,error,127)
			format(debugt,255,"Error Making Tables: '%s' - '%s'", error, sql)
			debugMessage(debugt,0,0)
			return
		}

		CreateTables = false
	}
}
//----------------------------------------------------------------------------------------------
// Flushes data in memory table position x to database...
public writeData(x)
{
	mySQLConnect()
	if ( gMySQL < 1 ) return

	debugMessage("Trying to save XP data to MySQL database", x, 8)

	new error[128],sql[512]

	new bkqtname[65]
	copy(bkqtname,64,gMemoryTableNames[x])
	replace_all(bkqtname,64,"`","\`")
	replace_all(bkqtname,64,"'","\'")

	//Thanks to HC for help with writing more efficient queries
	//Check if this user has an entry already, if not make one
	format(sql, 511, "SELECT * FROM `sh_savexp` WHERE `SH_KEY` = '%s'", gMemoryTableKeys[x] )
	if (mysql_query(gMySQL,sql) < 1) {
		mysql_error(gMySQL,error,127)
		format(debugt,255,"Error Querying MySQL DB for %s: '%s' - '%s'", gMemoryTableKeys[x], error, sql)
		debugMessage(debugt,0,0)
		return
	}
	else if (mysql_nextrow(gMySQL) < 1 ) {
		format(sql, 511, "INSERT INTO `sh_savexp` (SH_KEY) VALUES ('%s')", gMemoryTableKeys[x] )
		if (mysql_query(gMySQL,sql) < 1) {
			mysql_error(gMySQL,error,127)
			format(debugt,255,"Error Writing MySQL XP for %s: '%s' - '%s'", gMemoryTableKeys[x], error, sql)
			debugMessage(debugt,0,0)
			return
		}
	}

	//Update users entry with current data
	format(sql, 511, "UPDATE `sh_savexp` SET `PLAYER_NAME`='%s', `LAST_PLAY_DATE`=SYSDATE(), `XP`='%d', `HUDHELP`='%d', `SKILL_COUNT`='%d' WHERE (SH_KEY='%s')",bkqtname,gMemoryTableXP[x],gMemoryTableFlags[x],gMemoryTablePowers[x][0],gMemoryTableKeys[x] )
	if (mysql_query(gMySQL,sql) < 1) {
		mysql_error(gMySQL,error,127)
		format(debugt,255,"Error Writing MySQL XP for %s: '%s' - '%s'", gMemoryTableKeys[x], error, sql)
		debugMessage(debugt,0,0)
		return
	}

	if (!is_user_connected(x) || gChangedHeroes[x]) {

		//Remove all saved powers for this user
		format(sql,511, "DELETE FROM `sh_saveskills` WHERE SH_KEY='%s'",gMemoryTableKeys[x])
		if (mysql_query(gMySQL,sql) < 1) {
			mysql_error(gMySQL,error,127)
			format(debugt,255,"Error Deleting MySQL Skills for %s: '%s' - '%s'", gMemoryTableKeys[x], error, sql)
			debugMessage(debugt,0,0)
			return
		}

		// Saving by SuperHeroName since the hero order in the plugin.ini can change...
		for (new p = 1; p <= gMemoryTablePowers[x][0]; p++) {
			new heroIndex = gMemoryTablePowers[x][p]
			format(sql, 511, "INSERT INTO `sh_saveskills` VALUES ('%s','%d','%s')", gMemoryTableKeys[x],p,gSuperHeros[heroIndex][hero] )
			if (mysql_query(gMySQL,sql) < 1) {
				mysql_error(gMySQL,error,127)
				format(debugt,255,"Error Writing MySQL XP for %s: '%s' - '%s'", gMemoryTableKeys[x], error, sql)
				debugMessage(debugt,0,0)
				return
			}
		}

		if (x > 0 && x <= SH_MAXSLOTS) {
			gChangedHeroes[x] = false
		}
	}

	//Closes the connection if set not to be persistent
	close_mysql()
}
//----------------------------------------------------------------------------------------------
public loadXP( id, key[] )
{
	mySQLConnect()
	if ( gMySQL < 1 ) return false

	debugMessage("Trying to load XP data from MySQL database", id, 8)

	new sql[512], error[128]
	new xp[32], skillCount[10], userFlags[64]
	new skills = 0

	format(sql, 511, "SELECT `XP`, `HUDHELP`, `SKILL_COUNT` FROM `sh_savexp` WHERE `SH_KEY` = '%s'", key)
	if (mysql_query(gMySQL,sql) < 1) {
		mysql_error(gMySQL,error,127)
		format(debugt,127,"No Saved XP Loaded: %s:'%s'",key,error)
		debugMessage(debugt,id)

		//Closes the connection if set not to be persistent
		close_mysql()

		return true
	}

	if (mysql_nextrow(gMySQL) > 0) {

		mysql_getfield(gMySQL, 1, xp, 31)
		gPlayerXP[id] = str_to_num(xp)
		gPlayerLevel[id]=getLevel(id)
		setLevel(id, gPlayerLevel[id] )

		mysql_getfield(gMySQL, 2, userFlags, 63)
		gPlayerFlags[id] = str_to_num(userFlags)

		mysql_getfield(gMySQL, 3, skillCount, 9)

		format(sql, 511, "SELECT `HERO_NAME` FROM `sh_saveskills` WHERE `SH_KEY` = '%s' AND `SKILL_NUMBER` <= '%s' ORDER BY `SKILL_NUMBER` ASC", key, skillCount )

		if (mysql_query(gMySQL,sql) < 1) {
			mysql_error(gMySQL,error,127)
			format(debugt,255,"Error Loading MySQL XP for %s: '%s' - '%s'", key, error, sql)
			debugMessage(debugt,0,0)
			return false
		}

		gPlayerPowers[id][0]=0
		while ( mysql_nextrow(gMySQL) > 0 ) {
			new heroName[25]
			mysql_getfield(gMySQL, 1, heroName, 24)
			new heroIndex=getHeroIndex(heroName)
			if ( heroIndex>=0 && getHeroLevel(heroIndex)<=gPlayerLevel[id] ) {
				gPlayerPowers[id][0]=++skills
				gPlayerPowers[id][skills]=heroIndex
				initHero(id, heroIndex)
			}
		}
		updateMemoryTable(id)
	}

	//Closes the connection if set not to be persistent
	close_mysql()

	return true
}
//----------------------------------------------------------------------------------------------
public cleanXP( bool:clearAll )
{
	if ( !clearAll && (!get_cvar_num("sh_savexp") || get_cvar_num("sh_xpsavedays") < 0)) return

	new error[127], sql[255]

	mySQLConnect()
	if ( gMySQL < 1) return

	if (clearAll) {
		format(sql, 254, "TRUNCATE TABLE `sh_saveskills`")
		if (mysql_query(gMySQL, sql) < 1) {
			mysql_error(gMySQL,error,127)
			format(debugt,255,"Couldn't Erase XP: '%s' - '%s' - %d", error, sql, gMySQL)
			debugMessage(debugt,0,0)
			return
		}
		format(sql, 254, "TRUNCATE TABLE `sh_savexp`")
		if (mysql_query(gMySQL, sql) < 1) {
			mysql_error(gMySQL,error,127)
			format(debugt,255,"Couldn't Erase XP: '%s' - '%s' - %d", error, sql, gMySQL)
			debugMessage(debugt,0,0)
			return
		}
	}
	else {

		#if defined NEW_MYSQL

		format(sql, 254, "DELETE FROM `sh_saveskills` USING `sh_saveskills`, `sh_savexp` WHERE sh_savexp.`SH_KEY` = sh_saveskills.`SH_KEY` AND sh_savexp.`LAST_PLAY_DATE` < (SYSDATE() - INTERVAL '%d' DAY)", get_cvar_num("sh_xpsavedays") )
		if (mysql_query(gMySQL, sql) < 1) {
			mysql_error(gMySQL,error,127)
			format(debugt,255,"Couldn't Clean Powers: '%s' - '%s'", error, sql)
			debugMessage(debugt,0,0)
			return
		}

		format(sql, 254, "DELETE FROM `sh_savexp` WHERE `LAST_PLAY_DATE` < (SYSDATE() - INTERVAL '%d' DAY)", get_cvar_num("sh_xpsavedays") )
		if (mysql_query(gMySQL, sql) < 1) {
			mysql_error(gMySQL,error,127)
			format(debugt,255,"Couldn't Clean XP: '%s' - '%s'", error, sql)
			debugMessage(debugt,0,0)
			return
		}

		#else

		format(sql, 254, "SELECT `SH_KEY` FROM `sh_savexp` WHERE `LAST_PLAY_DATE` < (SYSDATE() - INTERVAL '%d' DAY)", get_cvar_num("sh_xpsavedays") )
		if (mysql_query(gMySQL, sql) < 1) {
			mysql_error(gMySQL,error,127)
			format(debugt,255,"Couldn't SELECT to Clean XP: '%s' - '%s'", error, sql)
			debugMessage(debugt,0,0)
			return
		}

		while (mysql_nextrow(gMySQL) > 0) {
			new sh_key[65]
			mysql_getfield(gMySQL, 1, sh_key, 64)
			if (strlen(sh_key) <=0) return

			//Need to escape in case we get some bot names
			replace_all(sh_key,64,"`","\`")
			replace_all(sh_key,64,"'","\'")

			format(sql,511, "DELETE FROM `sh_savexp` WHERE SH_KEY='%s'",sh_key)
			if (mysql_query(gMySQL, sql) < 1) {
				mysql_error(gMySQL,error,127)
				format(debugt,255,"Couldn't Clean XP: '%s' - '%s'", error, sql)
				debugMessage(debugt,0,0)
				return
			}

			format(sql,511, "DELETE FROM `sh_saveskills` WHERE SH_KEY='%s'",sh_key)
			if (mysql_query(gMySQL, sql) < 1) {
				mysql_error(gMySQL,error,127)
				format(debugt,255,"Couldn't Clean Powers: '%s' - '%s'", error, sql)
				debugMessage(debugt,0,0)
				return
			}
		}

		#endif

	}

	//Closes the connection if set not to be persistent
	close_mysql()
}
//----------------------------------------------------------------------------------------------
public close_mysql()
{
	if (gMySQL < 1 || gPersistent || gPersistentTemp) return
	mysql_close(gMySQL)
	gMySQL = 0
}
//----------------------------------------------------------------------------------------------
public saving_end()
{
	if ( gMySQL < 1 ) return
	mysql_close(gMySQL)
	gMySQL = 0
}
//----------------------------------------------------------------------------------------------
#endif