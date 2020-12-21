Player model changer 

Exmple use
```
#pragma newdecls required

// return:
// Plugin_Continue - has no effect
// Plugin_Changed - uses the specified parameters for inventory model
// Plugin_Handled & Plugin_Stop - force to use common model insted inventory model
forward Action MdlCh_PlayerSpawn(int client, bool custom, char[] model, int model_maxlen, char[] vo_prefix, int prefix_maxlen);

public Extension __ext_modelch = 
{
	name = "modelch",
	file = "modelch.ext",
#if defined AUTOLOAD_EXTENSIONS
	autoload = 1,
#else
	autoload = 0,
#endif
#if defined REQUIRE_EXTENSIONS
	required = 1,
#else
	required = 0,
#endif
};

public Action MdlCh_PlayerSpawn(int client, bool custom, char[] model, int model_maxlen, char[] vo_prefix, int prefix_maxlen)
{
	//PrintToServer("[MdlCtrl] MdlCh_PlayerSpawn(%i, %i, \"%s\", %i,\"%s\", %i)", client, custom, model, model_maxlen, vo_prefix, prefix_maxlen);
	
	if (custom)
		return Plugin_Continue;
	
	if (GetClientTeam(client) == 3)
	{
		strcopy(model, model_maxlen, "models/player/custom_player/legacy/ctm_fbi_variantb.mdl");
		strcopy(vo_prefix, prefix_maxlen, "fbihrt_epic");
	}
	else
	{
		strcopy(model, model_maxlen, "models/player/custom_player/legacy/tm_balkan_varianth.mdl");
		strcopy(vo_prefix, prefix_maxlen, "balkan_epic");
	}

	return Plugin_Changed;
}
```