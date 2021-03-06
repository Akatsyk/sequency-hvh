#pragma once

enum class_id {
	cai_basenpc = 0,
	cak47,
	cbaseanimating,
	cbaseanimatingoverlay,
	cbaseattributableitem,
	cbasebutton,
	cbasecombatcharacter,
	cbasecombatweapon,
	cbasecsgrenade,
	cbasecsgrenadeprojectile,
	cbasedoor,
	cbaseentity,
	cbaseflex,
	cbasegrenade,
	cbaseparticleentity,
	cbaseplayer,
	cbasepropdoor,
	cbaseteamobjectiveresource,
	cbasetempentity,
	cbasetoggle,
	cbasetrigger,
	cbaseviewmodel,
	cbasevphysicstrigger,
	cbaseweaponworldmodel,
	cbeam,
	cbeamspotlight,
	cbonefollower,
	cbrc4target,
	cbreachcharge,
	cbreachchargeprojectile,
	cbreakableprop,
	cbreakablesurface,
	cbumpmine,
	cbumpmineprojectile,
	cc4,
	ccascadelight,
	cchicken,
	ccolorcorrection,
	ccolorcorrectionvolume,
	ccsgamerulesproxy,
	ccsplayer,
	ccsplayerresource,
	ccsragdoll,
	ccsteam,
	cdangerzone,
	cdangerzonecontroller,
	cdeagle,
	cdecoygrenade,
	cdecoyprojectile,
	cdrone,
	cdronegun,
	cdynamiclight,
	cdynamicprop,
	ceconentity,
	ceconwearable,
	cembers,
	centitydissolve,
	centityflame,
	centityfreezing,
	centityparticletrail,
	cenvambientlight,
	cenvdetailcontroller,
	cenvdofcontroller,
	cenvgascanister,
	cenvparticlescript,
	cenvprojectedtexture,
	cenvquadraticbeam,
	cenvscreeneffect,
	cenvscreenoverlay,
	cenvtonemapcontroller,
	cenvwind,
	cfeplayerdecal,
	cfirecrackerblast,
	cfiresmoke,
	cfiretrail,
	cfish,
	cfists,
	cflashbang,
	cfogcontroller,
	cfootstepcontrol,
	cfunc_dust,
	cfunc_lod,
	cfuncareaportalwindow,
	cfuncbrush,
	cfuncconveyor,
	cfuncladder,
	cfuncmonitor,
	cfuncmovelinear,
	cfuncoccluder,
	cfuncreflectiveglass,
	cfuncrotating,
	cfuncsmokevolume,
	cfunctracktrain,
	cgamerulesproxy,
	cgrassburn,
	chandletest,
	chegrenade,
	chostage,
	chostagecarriableprop,
	cincendiarygrenade,
	cinferno,
	cinfoladderdismount,
	cinfomapregion,
	cinfooverlayaccessor,
	citem_healthshot,
	citemcash,
	citemdogtags,
	cknife,
	cknifegg,
	clightglow,
	cmaterialmodifycontrol,
	cmelee,
	cmolotovgrenade,
	cmolotovprojectile,
	cmoviedisplay,
	cparadropchopper,
	cparticlefire,
	cparticleperformancemonitor,
	cparticlesystem,
	cphysbox,
	cphysboxmultiplayer,
	cphysicsprop,
	cphysicspropmultiplayer,
	cphysmagnet,
	cphyspropammobox,
	cphysproplootcrate,
	cphyspropradarjammer,
	cphyspropweaponupgrade,
	cplantedc4,
	cplasma,
	cplayerping,
	cplayerresource,
	cpointcamera,
	cpointcommentarynode,
	cpointworldtext,
	cposecontroller,
	cpostprocesscontroller,
	cprecipitation,
	cprecipitationblocker,
	cpredictedviewmodel,
	cprop_hallucination,
	cpropcounter,
	cpropdoorrotating,
	cpropjeep,
	cpropvehicledriveable,
	cragdollmanager,
	cragdollprop,
	cragdollpropattached,
	cropekeyframe,
	cscar17,
	csceneentity,
	csensorgrenade,
	csensorgrenadeprojectile,
	cshadowcontrol,
	cslideshowdisplay,
	csmokegrenade,
	csmokegrenadeprojectile,
	csmokestack,
	csnowball,
	csnowballpile,
	csnowballprojectile,
	cspatialentity,
	cspotlightend,
	csprite,
	cspriteoriented,
	cspritetrail,
	cstatueprop,
	csteamjet,
	csun,
	csunlightshadowcontrol,
	csurvivalspawnchopper,
	ctablet,
	cteam,
	cteamplayroundbasedrulesproxy,
	ctearmorricochet,
	ctebasebeam,
	ctebeamentpoint,
	ctebeaments,
	ctebeamfollow,
	ctebeamlaser,
	ctebeampoints,
	ctebeamring,
	ctebeamringpoint,
	ctebeamspline,
	ctebloodsprite,
	ctebloodstream,
	ctebreakmodel,
	ctebspdecal,
	ctebubbles,
	ctebubbletrail,
	cteclientprojectile,
	ctedecal,
	ctedust,
	ctedynamiclight,
	cteeffectdispatch,
	cteenergysplash,
	cteexplosion,
	ctefirebullets,
	ctefizz,
	ctefootprintdecal,
	ctefoundryhelpers,
	ctegaussexplosion,
	cteglowsprite,
	cteimpact,
	ctekillplayerattachments,
	ctelargefunnel,
	ctemetalsparks,
	ctemuzzleflash,
	cteparticlesystem,
	ctephysicsprop,
	cteplantbomb,
	cteplayeranimevent,
	cteplayerdecal,
	cteprojecteddecal,
	cteradioicon,
	cteshattersurface,
	cteshowline,
	ctesla,
	ctesmoke,
	ctesparks,
	ctesprite,
	ctespritespray,
	ctest_proxytoggle_networkable,
	ctesttraceline,
	cteworlddecal,
	ctriggerplayermovement,
	ctriggersoundoperator,
	cvguiscreen,
	cvotecontroller,
	cwaterbullet,
	cwaterlodcontrol,
	cweaponaug,
	cweaponawp,
	cweaponbaseitem,
	cweaponbizon,
	cweaponcsbase,
	cweaponcsbasegun,
	cweaponcycler,
	cweaponelite,
	cweaponfamas,
	cweaponfiveseven,
	cweapong3sg1,
	cweapongalil,
	cweapongalilar,
	cweaponglock,
	cweaponhkp2000,
	cweaponm249,
	cweaponm3,
	cweaponm4a1,
	cweaponmac10,
	cweaponmag7,
	cweaponmp5navy,
	cweaponmp7,
	cweaponmp9,
	cweaponnegev,
	cweaponnova,
	cweaponp228,
	cweaponp250,
	cweaponp90,
	cweaponsawedoff,
	cweaponscar20,
	cweaponscout,
	cweaponsg550,
	cweaponsg552,
	cweaponsg556,
	cweaponshield,
	cweaponssg08,
	cweapontaser,
	cweapontec9,
	cweapontmp,
	cweaponump45,
	cweaponusp,
	cweaponxm1014,
	cweaponzonerepulsor,
	cworld,
	cworldvguitext,
	dusttrail,
	movieexplosion,
	particlesmokegrenade,
	rockettrail,
	smoketrail,
	sporeexplosion,
	sporetrail,
};

enum item_definition_index : short {
	weapon_none = 0,
	weapon_deagle = 1,
	weapon_elite = 2,
	weapon_fiveseven = 3,
	weapon_glock = 4,
	weapon_ak47 = 7,
	weapon_aug = 8,
	weapon_awp = 9,
	weapon_famas = 10,
	weapon_g3sg1 = 11,
	weapon_galilar = 13,
	weapon_m249 = 14,
	weapon_m4a1 = 16,
	weapon_mac10 = 17,
	weapon_p90 = 19,
	weapon_zone_repulsor,
	weapon_mp5sd = 23,
	weapon_ump45 = 24,
	weapon_xm1014 = 25,
	weapon_bizon = 26,
	weapon_mag7 = 27,
	weapon_negev = 28,
	weapon_sawedoff = 29,
	weapon_tec9 = 30,
	weapon_taser = 31,
	weapon_hkp2000 = 32,
	weapon_mp7 = 33,
	weapon_mp9 = 34,
	weapon_nova = 35,
	weapon_p250 = 36,
	weapon_shield = 37,
	weapon_scar20 = 38,
	weapon_sg556 = 39,
	weapon_ssg08 = 40,
	weapon_knifegg = 41,
	weapon_knife = 42,
	weapon_flashbang = 43,
	weapon_hegrenade = 44,
	weapon_smokegrenade = 45,
	weapon_molotov = 46,
	weapon_decoy = 47,
	weapon_incgrenade = 48,
	weapon_c4 = 49,
	item_kevlar = 50,
	item_assaultsuit = 51,
	item_heavyassaultsuit = 52,
	item_nvg = 54,
	item_defuser = 55,
	item_cutters = 56,
	weapon_healthshot = 57,
	musickit_default = 58,
	weapon_knife_t = 59,
	weapon_m4a1_silencer = 60,
	weapon_usp_silencer = 61,
	recipe_trade_up = 62,
	weapon_cz75a = 63,
	weapon_revolver = 64,
	weapon_tagrenade = 68,
	weapon_fists = 69,
	weapon_breachcharge = 70,
	weapon_tablet = 72,
	weapon_melee = 74,
	weapon_axe = 75,
	weapon_hammer = 76,
	weapon_spanner = 78,
	weapon_knife_ghost = 80,
	weapon_firebomb = 81,
	weapon_diversion = 82,
	weapon_frag_grenade = 83,
	weapon_snowball = 84,
	weapon_bumpmine = 85,
	weapon_bayonet = 500,
	weapon_knife_flip = 505,
	weapon_knife_gut = 506,
	weapon_knife_karambit = 507,
	weapon_knife_m9_bayonet = 508,
	weapon_knife_tactical = 509,
	weapon_knife_falchion = 512,
	weapon_knife_survival_bowie = 514,
	weapon_knife_butterfly = 515,
	weapon_knife_push = 516,
	weapon_knife_ursus = 519,
	weapon_knife_gypsy_jackknife = 520,
	weapon_knife_stiletto = 522,
	weapon_knife_widowmaker = 523,
	weapon_knife_skeleton = 525
};