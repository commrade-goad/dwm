#include <X11/XF86keysym.h>
#include "movestack.c"

/* some settings */
static unsigned int borderpx             = 2;   /* border pixel of windows */
static unsigned int snap                 = 32;  /* snap pixel */
static const unsigned int gappx          = 0;        /* gaps between windows */
static int showbar                       = 1;   /* 0 means no bar */
static int topbar                        = 1;   /* 0 means bottom bar */
static const int showsystray             = 1;   /* 0 means no systray */
static const unsigned int systraypinning = 0;   /* 0: sloppy systray follows selected monitor, >0: pin systray to monitor X */
static const unsigned int systrayonleft  = 0;   /* 0: systray in the right corner, >0: systray on left of status text */
static const unsigned int systrayspacing = 2;   /* systray spacing */
static const int systraypinningfailfirst = 1;   /* 1: if pinning fails, display systray on the first monitor, False: display systray on the last monitor*/
static const char *fonts[]               = { "monospace:size=10" };
static const char dmenufont[]            = "monospace:size=10";
static const int refreshrate             = 120;  /* refresh rate (per second) for client move/resize */
static char normbgcolor[]                = "#222222";
static char normbordercolor[]            = "#444444";
static char normfgcolor[]                = "#bbbbbb";
static char selfgcolor[]                 = "#eeeeee";
static char selbordercolor[]             = "#005577";
static char selbgcolor[]                 = "#005577";
static char *colors[][3]                 = {
       /*               fg           bg           border   */
       [SchemeNorm] = { normfgcolor, normbgcolor, normbordercolor },
       [SchemeSel]  = { selfgcolor,  selbgcolor,  selbordercolor  },
};

/* some commands */
static const char *up_vol[]       = { "pamixer-wrapper",       "raise",                  NULL };
static const char *down_vol[]     = { "pamixer-wrapper",       "lower",                  NULL };
static const char *mute_vol[]     = { "pamixer-wrapper",       "toggle",                 NULL };
static const char *mute_mic_vol[] = { "pamixer",               "--default-source", "-t", NULL };

static const char *brighter[]     = { "brightnessctl-wrapper", "raise",      NULL };
static const char *dimmer[]       = { "brightnessctl-wrapper", "lower",      NULL };
static const char *toggle_mpris[] = {"playerctl",              "play-pause", NULL };
static const char *next_mpris[]   = {"playerctl",              "next"      , NULL };
static const char *prev_mpris[]   = {"playerctl",              "previous"  , NULL };

static char dmenumon[2] = "0"; /* component of dmenucmd, manipulated in spawn() */
static const char *dmenucmd[] = { "dmenu_run", "-m", dmenumon, "-fn", dmenufont, "-nb", normbgcolor, "-nf", normfgcolor, "-sb", selbordercolor, "-sf", selfgcolor, NULL };
static const char *emoji[]    = { "dmenumoji", "-m", dmenumon, "-fn", dmenufont, "-nb", normbgcolor, "-nf", normfgcolor, "-sb", selbordercolor, "-sf", selfgcolor, NULL };
static const char *termcmd[]  = { "alacritty", NULL };

/* tagging */
static const char *tags[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9" };
// static const char *tags[] = { "一", "二", "三", "四", "五", "六", "七", "八", "九" };

/* some rule stuff */
static const Rule rules[] = {
	/* xprop(1):
	 *	WM_CLASS(STRING) = instance, class
	 *	WM_NAME(STRING) = title
	 */
	/* class      instance    title       tags mask     isfloating   monitor */
    { "mpv"         ,  NULL,       NULL,       0,            1,           -1 },
    { "firefox"     ,  NULL,       NULL,       1<<1,         0,           -1 },
    { "vesktop"     ,  NULL,       NULL,       1<<2,         0,           -1 },
    { "pavucontrol" ,  NULL,       NULL,       0,            1,           -1 },
};

/* layout(s) */
static float mfact        = 0.55; /* factor of master area size [0.05..0.95] */
static int nmaster        = 1;    /* number of clients in master area */
static int resizehints    = 1;    /* 1 means respect size hints in tiled resizals */
static const int lockfullscreen = 1; /* 1 will force focus on the fullscreen window */

static const Layout layouts[] = {
	/* symbol     arrange function */
	{ "[]=",      tile },    /* first entry is default */
	{ "><>",      NULL },    /* no layout function means floating behavior */
	{ "[M]",      monocle },
};

/* key definitions */
#define MODKEY Mod4Mask
#define TAGKEYS(KEY,TAG) \
	{ MODKEY,                       KEY,      view,           {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask,           KEY,      toggleview,     {.ui = 1 << TAG} }, \
	{ MODKEY|ShiftMask,             KEY,      tag,            {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask|ShiftMask, KEY,      toggletag,      {.ui = 1 << TAG} },

/* helper for spawning shell commands in the pre dwm-5.0 fashion */
#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }

/*
 * Xresources preferences to load at startup
 */
ResourcePref resources[] = {
    { "normbgcolor",        STRING,  &normbgcolor },
    { "normbordercolor",    STRING,  &normbordercolor },
    { "normfgcolor",        STRING,  &normfgcolor },
    { "selbgcolor",         STRING,  &selbgcolor },
    { "selbordercolor",     STRING,  &selbordercolor },
    { "selfgcolor",         STRING,  &selfgcolor },
    { "borderpx",          	INTEGER, &borderpx },
    { "snap",          		INTEGER, &snap },
    { "showbar",          	INTEGER, &showbar },
    { "topbar",          	INTEGER, &topbar },
    /* { "nmaster",          	INTEGER, &nmaster }, */
    { "resizehints",       	INTEGER, &resizehints },
    /* { "mfact",      	 	FLOAT,   &mfact }, */
};

static const Key keys[] = {
	/* modifier                     key        function           argument */
	{ MODKEY,                       XK_p,      spawn,             {.v = dmenucmd } },
	{ MODKEY,                       XK_Return, spawn,             {.v = termcmd } },
	{ MODKEY,                       XK_b,      togglebar,         {0} },
	{ MODKEY|ShiftMask,             XK_r,      reload_xresources, {0} },
	{ MODKEY,                       XK_j,      focusstack,        {.i = +1 } },
	{ MODKEY,                       XK_k,      focusstack,        {.i = -1 } },
	{ MODKEY,                       XK_i,      incnmaster,        {.i = +1 } },
	{ MODKEY,                       XK_d,      incnmaster,        {.i = -1 } },
	{ MODKEY,                       XK_h,      setmfact,          {.f = -0.05} },
	{ MODKEY,                       XK_l,      setmfact,          {.f = +0.05} },
    { MODKEY|ShiftMask,             XK_j,      movestack,         {.i = +1 } },
    { MODKEY|ShiftMask,             XK_k,      movestack,         {.i = -1 } },
	{ MODKEY,                       XK_Tab,    view,              {0} },
	{ MODKEY|ShiftMask,             XK_q,      killclient,        {0} },
	{ MODKEY,                       XK_t,      setlayout,         {.v = &layouts[0]} },
	{ MODKEY,                       XK_f,      setlayout,         {.v = &layouts[1]} },
	{ MODKEY,                       XK_m,      setlayout,         {.v = &layouts[2]} },
	{ MODKEY,                       XK_space,  setlayout,         {0} },
	{ MODKEY|ShiftMask,             XK_space,  togglefloating,    {0} },
	{ MODKEY,                       XK_0,      view,              {.ui = ~0 } },
	{ MODKEY|ShiftMask,             XK_0,      tag,               {.ui = ~0 } },
	{ MODKEY,                       XK_comma,  focusmon,          {.i = -1 } },
	{ MODKEY,                       XK_period, focusmon,          {.i = +1 } },
	{ MODKEY|ShiftMask,             XK_comma,  tagmon,            {.i = -1 } },
	{ MODKEY|ShiftMask,             XK_period, tagmon,            {.i = +1 } },
	{ MODKEY,                       XK_minus,  setgaps,           {.i = -1 } },
	{ MODKEY,                       XK_equal,  setgaps,           {.i = +1 } },
	{ MODKEY|ShiftMask,             XK_equal,  _setgapsfr,        {.i = gappx  } },


    { 0,                  XF86XK_AudioMicMute,              spawn,         {.v = mute_mic_vol } },
    { 0,                  XF86XK_AudioMute,                 spawn,         {.v = mute_vol } },
    { 0,                  XF86XK_AudioLowerVolume,          spawn,         {.v = down_vol } },
    { 0,                  XF86XK_AudioRaiseVolume,          spawn,         {.v = up_vol } },
    { 0,                  XF86XK_MonBrightnessDown,         spawn,         {.v = dimmer } },
    { 0,                  XF86XK_MonBrightnessUp,           spawn,         {.v = brighter } },
    { 0,                  XF86XK_AudioPlay,                 spawn,         {.v = toggle_mpris} },
    { 0,                  XF86XK_AudioNext,                 spawn,         {.v = next_mpris} },
    { 0,                  XF86XK_AudioPrev,                 spawn,         {.v = prev_mpris} },
    { MODKEY|ShiftMask,   XK_f,                             togglefullscr, {0} },
    { MODKEY,             XK_period,                        spawn,         {.v = emoji } },
    { MODKEY|ShiftMask,   XK_p,                             spawn,         {.v = toggle_mpris} },
    { MODKEY,             XK_s,                             togglesticky,  {0} },
    { 0,                  XK_Print,                         spawn,         SHCMD("xsc.sh") },
    { ShiftMask,          XK_Print,                         spawn,         SHCMD("xscsel.sh") },
    { ControlMask|MODKEY, XK_space,                         spawn,         SHCMD("~/.local/share/bin/togglekb.sh") },


	TAGKEYS(                        XK_1,                      0)
	TAGKEYS(                        XK_2,                      1)
	TAGKEYS(                        XK_3,                      2)
	TAGKEYS(                        XK_4,                      3)
	TAGKEYS(                        XK_5,                      4)
	TAGKEYS(                        XK_6,                      5)
	TAGKEYS(                        XK_7,                      6)
	TAGKEYS(                        XK_8,                      7)
	TAGKEYS(                        XK_9,                      8)
	{ MODKEY|ShiftMask,             XK_c,      quit,           {0} },
};

/* button definitions */
/* click can be ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
static const Button buttons[] = {
	/* click                event mask      button          function        argument */
	{ ClkLtSymbol,          0,              Button1,        setlayout,      {0} },
	{ ClkLtSymbol,          0,              Button3,        setlayout,      {.v = &layouts[2]} },
	{ ClkWinTitle,          0,              Button2,        zoom,           {0} },
	{ ClkStatusText,        0,              Button2,        spawn,          {.v = termcmd } },
	{ ClkClientWin,         MODKEY,         Button1,        movemouse,      {0} },
	{ ClkClientWin,         MODKEY,         Button2,        togglefloating, {0} },
	{ ClkClientWin,         MODKEY,         Button3,        resizemouse,    {0} },
	{ ClkTagBar,            0,              Button1,        view,           {0} },
	{ ClkTagBar,            0,              Button3,        toggleview,     {0} },
	{ ClkTagBar,            MODKEY,         Button1,        tag,            {0} },
	{ ClkTagBar,            MODKEY,         Button3,        toggletag,      {0} },
};
