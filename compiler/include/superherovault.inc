// SuperHero Vault Include File
// Version 1.18e

//----------------------------------------------------------------------------------------------
// Abstract Methods that must be implemented by a save superhero include file...
//
// writeMemoryTable()
// loadXP( id, key[35] )
// cleanXP(clearAll = false)
//----------------------------------------------------------------------------------------------

#if defined _shsave_included || !defined SHCORE
  #endinput
#endif
#define _shsave_included

// Global to this save class
new gVaultFile[128]
//----------------------------------------------------------------------------------------------
public saving_init()
{
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
public parseXP(id, const data[] )
{
	// "name" epochtime XP HUDHELP "SKILL1" "SKILL2" ...
	new heroIndex
	new Left[32], Right[1501]

	copy(Right, 1500, data)

	//Get the name out
	strbrkqt(Right, Left, 31, Right, 1500)

	//Get the epochtime out
	strbrkqt(Right, Left, 31, Right, 1500)

	//Get the XP out and save it
	strbrkqt(Right, Left, 31, Right, 1500)
	gPlayerXP[id] = str_to_num(Left)
	gPlayerLevel[id] = getLevel(id)
	setLevel(id, gPlayerLevel[id] )

	//Get the User Flags out and save them
	strbrkqt(Right, Left, 31, Right, 1500)
	gPlayerFlags[id] = str_to_num(Left)

	//Now load in all the skills
	new powerCount = 0
	new MaxPowers = min(get_cvar_num("sh_maxpowers"), SH_MAXLEVELS)

	while ( strlen(Right) > 0 && powerCount < MaxPowers ) {
		strbrkqt(Right, Left, 31, Right, 1500)
		heroIndex = findHero(Left)
		if ( heroIndex >= 0 )  {
			if ( getHeroLevel(heroIndex) <= gPlayerLevel[id] ) {
				gPlayerPowers[id][0] = ++powerCount
				gPlayerPowers[id][powerCount] = heroIndex
				initHero(id, heroIndex)
			}
		}
	}
}
//----------------------------------------------------------------------------------------------
public writeMemoryTable()
{
	if ( !get_cvar_num("sh_savexp") ) return

	debugMessage("Writing XP Data to Vault file",0,3)

	// Write Memory Table to File
	for (new x = 1; x < gMemoryTableCount; x++) {
		if ( strlen(gMemoryTableKeys[x]) > 0 ) {
			writeData(x)
		}
		// Can even clear the MemoryKey - if a player is disconnected no reason to save again and again...
		copy(gMemoryTableKeys[x], 34, "")
	}

	// No need to keep disconnected players around if this has been flushed...
	gMemoryTableCount = 33
}
//----------------------------------------------------------------------------------------------
public writeData(id)
{
	// Formats Player Information and writes to file:
	// "name" epochtime XP USERFLAGS "SKILL1" "SKILL2" ...
	new data[1501], heroIndex, vaultSaveKey[40]

	format(data, 1500, "^"%s^" %d %d %d", gMemoryTableNames[id], get_systime(), gMemoryTableXP[id], gMemoryTableFlags[id] )
	for ( new x = 1; x <= gMemoryTablePowers[id][0] && x < gSuperHeroCount; x++ ) {
		heroIndex = gMemoryTablePowers[id][x]
		if ( heroIndex >= 0 && heroIndex < gSuperHeroCount ) {
			add(data, 1500, " ^"")
			add(data, 1500, gSuperHeros[heroIndex][hero])
			add(data, 1500, "^"")
		}
	}
	format(vaultSaveKey,39,"shinfo.%s",gMemoryTableKeys[id])
	set_vaultdata(vaultSaveKey, data)

	if (id > 0 && id <= SH_MAXSLOTS) {
		gChangedHeroes[id] = false
	}
}
//----------------------------------------------------------------------------------------------
public loadXP( id, key[35] )
{
	new data[1501], vaultSaveKey[40]
	format(vaultSaveKey,39,"shinfo.%s",key)
	get_vaultdata(vaultSaveKey, data, 1500)

	if ( strlen(data) > 12 ) {
		parseXP(id, data)
	}

	updateMemoryTable(id)
	return true
}
//----------------------------------------------------------------------------------------------
public cleanXP(bool:clearAll)
{
	if ( !clearAll && (!get_cvar_num("sh_savexp")) || get_cvar_num("sh_xpsavedays") < 0) return

	// Open up the vault file - read line by line
	// Use vault to delete to any data over a certain age
	if ( !file_exists(gVaultFile) ) return

	// Read through the file looking for shinfo entries - Check the epoch time to see if deletion should happen
	new lengthRead, nextLine = 0
	new data[256], key[35], name[32], epoch[16]

	// This is an imperfect clean - but gets the job done
	// Vault needs to have a way to walk it
	// Deleting a Key can actually reorder the vault.ini file
	while ((nextLine = read_file(gVaultFile,nextLine,data,255,lengthRead)) != 0) {
		if ( lengthRead > 0 && containi(data, "shinfo") == 0 ) {
			parse(data, key, 34, name, 31, epoch, 15)
			if ( clearAll || str_to_num(epoch) + (get_cvar_num("sh_xpsavedays") * 24 * 3600) < get_systime()) {
				remove_vaultdata( key )
				// since remove_vaultdata can reorder the whole file - we will go back some lines
				nextLine = max(nextLine - 1, 0)
			}
		}
	}
}
//----------------------------------------------------------------------------------------------
public saving_end()
{
	//Don't need to clean up vault saving
}
//----------------------------------------------------------------------------------------------