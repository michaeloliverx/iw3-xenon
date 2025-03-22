#include "xtl.h"

#pragma warning(disable : 4480)

namespace mp
{

    /* 9096 */
    struct EntHandle
    {
        unsigned __int16 number;
        unsigned __int16 infoIndex;
    };

    /* 9097 */
    struct entityShared_t
    {
        unsigned __int8 linked;
        unsigned __int8 bmodel;
        unsigned __int8 svFlags;
        int clientMask[2];
        unsigned __int8 inuse;
        int broadcastTime;
        float mins[3];
        float maxs[3];
        int contents;
        float absmin[3];
        float absmax[3];
        float currentOrigin[3];
        float currentAngles[3];
        EntHandle ownerNum;
        int eventTime;
    };

    static_assert(sizeof(entityShared_t) == 0x0068, "");

    /* 662 */
    enum OffhandSecondaryClass : __int32
    {
        PLAYER_OFFHAND_SECONDARY_SMOKE = 0x0,
        PLAYER_OFFHAND_SECONDARY_FLASH = 0x1,
        PLAYER_OFFHAND_SECONDARIES_TOTAL = 0x2,
    };

    /* 663 */
    enum ViewLockTypes : __int32
    {
        PLAYERVIEWLOCK_NONE = 0x0,
        PLAYERVIEWLOCK_FULL = 0x1,
        PLAYERVIEWLOCK_WEAPONJITTER = 0x2,
        PLAYERVIEWLOCKCOUNT = 0x3,
    };

    /* 665 */
    enum team_t : __int32
    {
        TEAM_FREE = 0x0,
        TEAM_AXIS = 0x1,
        TEAM_ALLIES = 0x2,
        TEAM_SPECTATOR = 0x3,
        TEAM_NUM_TEAMS = 0x4,
    };

    /* 8733 */
    struct SprintState
    {
        int sprintButtonUpRequired;
        int sprintDelay;
        int lastSprintStart;
        int lastSprintEnd;
        int sprintStartMaxLength;
    };

    /* 8734 */
    struct MantleState
    {
        float yaw;
        int timer;
        int transIndex;
        int flags;
    };

    /* 664 */
    enum ActionSlotType : __int32
    {
        ACTIONSLOTTYPE_DONOTHING = 0x0,
        ACTIONSLOTTYPE_SPECIFYWEAPON = 0x1,
        ACTIONSLOTTYPE_ALTWEAPONTOGGLE = 0x2,
        ACTIONSLOTTYPE_NIGHTVISION = 0x3,
        ACTIONSLOTTYPECOUNT = 0x4,
    };

    /* 8721 */
    struct ActionSlotParam_SpecifyWeapon
    {
        unsigned int index;
    };

    /* 8735 */
    struct ActionSlotParam
    {
        ActionSlotParam_SpecifyWeapon specifyWeapon;
    };

    /* 660 */
    enum objectiveState_t : __int32
    {
        OBJST_EMPTY = 0x0,
        OBJST_ACTIVE = 0x1,
        OBJST_INVISIBLE = 0x2,
        OBJST_DONE = 0x3,
        OBJST_CURRENT = 0x4,
        OBJST_FAILED = 0x5,
        OBJST_NUMSTATES = 0x6,
    };

    /* 8736 */
    struct objective_t
    {
        objectiveState_t state;
        float origin[3];
        int entNum;
        int teamNum;
        int icon;
    };

    /* 667 */
    enum he_type_t : __int32
    {
        HE_TYPE_FREE = 0x0,
        HE_TYPE_TEXT = 0x1,
        HE_TYPE_VALUE = 0x2,
        HE_TYPE_PLAYERNAME = 0x3,
        HE_TYPE_MAPNAME = 0x4,
        HE_TYPE_GAMETYPE = 0x5,
        HE_TYPE_MATERIAL = 0x6,
        HE_TYPE_TIMER_DOWN = 0x7,
        HE_TYPE_TIMER_UP = 0x8,
        HE_TYPE_TENTHS_TIMER_DOWN = 0x9,
        HE_TYPE_TENTHS_TIMER_UP = 0xA,
        HE_TYPE_CLOCK_DOWN = 0xB,
        HE_TYPE_CLOCK_UP = 0xC,
        HE_TYPE_WAYPOINT = 0xD,
        HE_TYPE_COUNT = 0xE,
    };

    /* 8713 */
    struct $0D0CB43DF22755AD856C77DD3F304010
    {
        unsigned __int8 r;
        unsigned __int8 g;
        unsigned __int8 b;
        unsigned __int8 a;
    };

    /* 8714 */
    union hudelem_color_t
    {
        $0D0CB43DF22755AD856C77DD3F304010 __s0;
        int rgba;
    };

    /* 8737 */
    struct hudelem_s
    {
        he_type_t type;
        float x;
        float y;
        float z;
        int targetEntNum;
        float fontScale;
        int font;
        int alignOrg;
        int alignScreen;
        hudelem_color_t color;
        hudelem_color_t fromColor;
        int fadeStartTime;
        int fadeTime;
        int label;
        int width;
        int height;
        int materialIndex;
        int offscreenMaterialIdx;
        int fromWidth;
        int fromHeight;
        int scaleStartTime;
        int scaleTime;
        float fromX;
        float fromY;
        int fromAlignOrg;
        int fromAlignScreen;
        int moveStartTime;
        int moveTime;
        int time;
        int duration;
        float value;
        int text;
        float sort;
        hudelem_color_t glowColor;
        int fxBirthTime;
        int fxLetterTime;
        int fxDecayStartTime;
        int fxDecayDuration;
        int soundID;
        int flags;
    };

    struct hudElemState_t
    {
        hudelem_s current[31];
        hudelem_s archival[31];
    };

    struct playerState_s
    {
        int commandTime; // 0
        int pm_type;     // 4
        int bobCycle;    // 8
        int pm_flags;    // 12
        int weapFlags;
        int otherFlags;
        int pm_time;
        float origin[3];
        float velocity[3]; // 40
        float oldVelocity[2];
        int weaponTime;
        int weaponDelay;
        int grenadeTimeLeft;
        int throwBackGrenadeOwner;
        int throwBackGrenadeTimeLeft;
        int weaponRestrictKickTime;
        int foliageSoundTime;
        int gravity;
        float leanf;
        int speed;
        float delta_angles[3];
        int groundEntityNum; // 112
        float vLadderVec[3];
        int jumpTime;
        float jumpOriginZ; // 132
        int legsTimer;     // 136
        int legsAnim;      // 140
        int torsoTimer;    // 144
        int torsoAnim;
        int legsAnimDuration;
        int torsoAnimDuration;
        int damageTimer;
        int damageDuration;
        int flinchYawAnim; // 168
        char _pad[4];      // 172
        int movementDir;   // 176
        int eFlags;        // 180
        int eventSequence;
        int events[4];
        unsigned int eventParms[4];
        int oldEventSequence;
        int clientNum;
        int offHandIndex;
        OffhandSecondaryClass offhandSecondary;
        unsigned int weapon;
        int weaponstate; // 240
        unsigned int weaponShotCount;
        float fWeaponPosFrac;
        int adsDelayTime;
        int spreadOverride;
        int spreadOverrideState;
        int viewmodelIndex;
        float viewangles[3]; // 268
        int viewHeightTarget;
        float viewHeightCurrent;
        int viewHeightLerpTime;
        int viewHeightLerpTarget;
        int viewHeightLerpDown;
        float viewAngleClampBase[2];
        float viewAngleClampRange[2];
        int damageEvent;
        int damageYaw;
        int damagePitch;
        int damageCount;
        int stats[5];
        int ammo[128];
        int ammoclip[128];
        unsigned int weapons[4];
        unsigned int weaponold[4];
        unsigned int weaponrechamber[4];
        float proneDirection;
        float proneDirectionPitch;
        float proneTorsoPitch;
        ViewLockTypes viewlocked;
        int viewlocked_entNum;
        int cursorHint;
        int cursorHintString;
        int cursorHintEntIndex;
        int iCompassPlayerInfo;
        int radarEnabled;
        int locationSelectionInfo;
        SprintState sprintState;
        float fTorsoPitch;
        float fWaistPitch;
        float holdBreathScale;
        int holdBreathTimer;
        float moveSpeedScaleMultiplier;
        MantleState mantleState;
        float meleeChargeYaw;
        int meleeChargeDist;
        int meleeChargeTime;
        int perks;
        ActionSlotType actionSlotType[4];
        ActionSlotParam actionSlotParam[4];
        int entityEventSequence;
        int weapAnim;
        float aimSpreadScale;
        int shellshockIndex;
        int shellshockTime;
        int shellshockDuration;
        float dofNearStart;
        float dofNearEnd;
        float dofFarStart;
        float dofFarEnd;
        float dofNearBlur;
        float dofFarBlur;
        float dofViewmodelStart;
        float dofViewmodelEnd;
        int hudElemLastAssignedSoundID;
        objective_t objective[16];
        unsigned __int8 weaponmodels[128];
        int deltaTime;
        int killCamEntity;
        hudElemState_t hud;
    };

    static_assert(offsetof(playerState_s, velocity) == 40, "");
    static_assert(offsetof(playerState_s, groundEntityNum) == 112, "");
    static_assert(offsetof(playerState_s, jumpOriginZ) == 132, "");
    static_assert(offsetof(playerState_s, legsTimer) == 136, "");
    static_assert(offsetof(playerState_s, torsoTimer) == 144, "");
    static_assert(offsetof(playerState_s, flinchYawAnim) == 168, "");
    static_assert(offsetof(playerState_s, movementDir) == 176, "");
    static_assert(offsetof(playerState_s, eFlags) == 180, "");
    static_assert(offsetof(playerState_s, weaponstate) == 240, "");
    static_assert(offsetof(playerState_s, viewangles) == 268, "");

    struct playerTeamState_t
    {
        int location;
    };

    /* 8741 */
    struct clientState_s
    {
        int clientIndex;
        team_t team;
        int modelindex;
        int attachModelIndex[6];
        int attachTagIndex[6];
        char name[32];
        float maxSprintTimeMultiplier;
        int rank;
        int prestige;
        int perks;
        int voiceConnectivityBits;
        char clanAbbrev[8];
        int attachedVehEntNum;
        int attachedVehSlotIndex;
    };

    /* 770 */
    enum clientConnected_t : __int32
    {
        CON_DISCONNECTED = 0x0,
        CON_CONNECTING = 0x1,
        CON_CONNECTED = 0x2,
    };

    /* 771 */
    enum sessionState_t : __int32
    {
        SESS_STATE_PLAYING = 0x0,
        SESS_STATE_DEAD = 0x1,
        SESS_STATE_SPECTATOR = 0x2,
        SESS_STATE_INTERMISSION = 0x3,
    };

    /* 8748 */
    struct __declspec(align(2)) usercmd_s
    {
        int serverTime;
        int buttons;
        int angles[3];
        unsigned __int8 weapon;
        unsigned __int8 offHandIndex;
        char forwardmove;
        char rightmove;
        float meleeChargeYaw;
        unsigned __int8 meleeChargeDist;
        char selectedLocation[2];
    };

    static_assert(sizeof(usercmd_s) == 0x0020, "");

    /* 9100 */
    struct clientSession_t
    {
        sessionState_t sessionState; // correct
        int forceSpectatorClient;
        int killCamEntity;
        int status_icon;
        int archiveTime;
        int score;
        int deaths;
        int kills;
        int assists;
        unsigned __int16 scriptPersId;
        clientConnected_t connected;
        usercmd_s cmd;
        usercmd_s oldcmd;
        int localClient;
        int predictItemPickup;
        char newnetname[32];
        int maxHealth;
        int enterTime;
        playerTeamState_t teamState;
        int voteCount;
        int teamVoteCount;
        float moveSpeedScaleMultiplier;
        int viewmodelIndex;
        int noSpectate;
        int teamInfo;
        clientState_s cs;
        int psOffsetTime;
    };

    struct gentity_s;

    struct gclient_s
    {
        playerState_s ps;
        clientSession_t sess;
        int spectatorClient;
        int noclip; // 0x30a8
        int ufo;    // 0x30ac
        int bFrozen;
        int lastCmdTime;
        int buttons;
        int oldbuttons;
        int latched_buttons;
        int buttonsSinceLastFrame;
        float oldOrigin[3];
        float fGunPitch;
        float fGunYaw;
        int damage_blood;
        float damage_from[3];
        int damage_fromWorld;
        int accurateCount;
        int accuracy_shots;
        int accuracy_hits;
        int inactivityTime;
        int inactivityWarning;
        int lastVoiceTime;
        int switchTeamTime;
        float currentAimSpreadScale;
        gentity_s *persistantPowerup;
        int portalID;
        int dropWeaponTime;
        int sniperRifleFiredTime;
        float sniperRifleMuzzleYaw;
        int PCSpecialPickedUpCount;
        EntHandle useHoldEntity;
        int useHoldTime;
        int useButtonDone;
        int iLastCompassPlayerInfoEnt;
        int compassPingTime;
        int damageTime;
        float v_dmg_roll;
        float v_dmg_pitch;
        float swayViewAngles[3];
        float swayOffset[3];
        float swayAngles[3];
        float vLastMoveAng[3];
        float fLastIdleFactor;
        float vGunOffset[3];
        float vGunSpeed[3];
        int weapIdleTime;
        int lastServerTime;
        int lastSpawnTime;
        unsigned int lastWeapon;
        bool previouslyFiring;
        bool previouslyUsingNightVision;
        bool previouslySprinting;
        int hasRadar;
        int lastStand;
        int lastStandTime;
    };

    static_assert(offsetof(gclient_s, noclip) == 0x30a8, "");
    static_assert(offsetof(gclient_s, ufo) == 0x30ac, "");
    static_assert(sizeof(gclient_s) == 12724, "");

    static_assert(offsetof(gclient_s, sess) + offsetof(clientSession_t, cmd) == 12180, "");
    static_assert(offsetof(gclient_s, sess) + offsetof(clientSession_t, archiveTime) == 12152, "");

    struct LerpEntityStatePhysicsJitter
    {
        float innerRadius;
        float minDisplacement;
        float maxDisplacement;
    };

    struct LerpEntityStatePlayer
    {
        float leanf;
        int movementDir;
    };

    struct LerpEntityStateLoopFx
    {
        float cullDist;
        int period;
    };

    struct LerpEntityStateCustomExplode
    {
        int startTime;
    };

    struct LerpEntityStateTurret
    {
        float gunAngles[3];
    };

    struct LerpEntityStateAnonymous
    {
        int data[7];
    };

    struct LerpEntityStateExplosion
    {
        float innerRadius;
        float magnitude;
    };

    struct LerpEntityStateBulletHit
    {
        float start[3];
    };

    struct LerpEntityStatePrimaryLight
    {
        byte colorAndExp[4];
        float intensity;
        float radius;
        float cosHalfFovOuter;
        float cosHalfFovInner;
    };

    struct LerpEntityStateMissile
    {
        int launchTime;
    };

    struct LerpEntityStateSoundBlend
    {
        float lerp;
    };

    struct LerpEntityStateExplosionJolt
    {
        float innerRadius;
        float impulse[3];
    };

    struct LerpEntityStateVehicle
    {
        float bodyPitch;
        float bodyRoll;
        float steerYaw;
        int materialTime;
        float gunPitch;
        float gunYaw;
        int team;
    };

    struct LerpEntityStateEarthquake
    {
        float scale;
        float radius;
        int duration;
    };

    /* 678 */
    enum trType_t : __int32
    {
        TR_STATIONARY = 0x0,
        TR_INTERPOLATE = 0x1,
        TR_LINEAR = 0x2,
        TR_LINEAR_STOP = 0x3,
        TR_SINE = 0x4,
        TR_GRAVITY = 0x5,
        TR_ACCELERATE = 0x6,
        TR_DECELERATE = 0x7,
        TR_PHYSICS = 0x8,
        TR_FIRST_RAGDOLL = 0x9,
        TR_RAGDOLL = 0x9,
        TR_RAGDOLL_GRAVITY = 0xA,
        TR_RAGDOLL_INTERPOLATE = 0xB,
        TR_LAST_RAGDOLL = 0xB,
    };

    /* 8750 */
    struct trajectory_t
    {
        trType_t trType;
        int trTime;
        int trDuration;
        float trBase[3];
        float trDelta[3];
    };

    /* 8743 */
    union LerpEntityStateTypeUnion
    {
        LerpEntityStateTurret turret;
        LerpEntityStateLoopFx loopFx;
        LerpEntityStatePrimaryLight primaryLight;
        LerpEntityStatePlayer player;
        LerpEntityStateVehicle vehicle;
        LerpEntityStateMissile missile;
        LerpEntityStateSoundBlend soundBlend;
        LerpEntityStateBulletHit bulletHit;
        LerpEntityStateEarthquake earthquake;
        LerpEntityStateCustomExplode customExplode;
        LerpEntityStateExplosion explosion;
        LerpEntityStateExplosionJolt explosionJolt;
        LerpEntityStatePhysicsJitter physicsJitter;
        LerpEntityStateAnonymous anonymous;
    };

    /* 8751 */
    struct LerpEntityState
    {
        int eFlags;
        trajectory_t pos;
        trajectory_t apos;
        LerpEntityStateTypeUnion u;
    };

    struct entityState_s
    {
        int number; // entity index	//0x00
        int eType;  // entityType_t	//0x04

        LerpEntityState lerp;

        int time2; // 0x70

        int otherEntityNum;    // 0x74 shotgun sources, etc
        int attackerEntityNum; // 0x78

        int groundEntityNum; // 0x7c -1 = in air

        int loopSound; // 0x80 constantly loop this sound
        int surfType;  // 0x84

        int index;         // 0x88
        int clientNum;     // 0x8c 0 to (MAX_CLIENTS - 1), for players and corpses
        int iHeadIcon;     // 0x90
        int iHeadIconTeam; // 0x94

        int solid; // 0x98 for client side prediction, trap_linkentity sets this properly	0x98

        int eventParm;     // 0x9c impulse events -- muzzle flashes, footsteps, etc
        int eventSequence; // 0xa0

        int events[4];     // 0xa4
        int eventParms[4]; // 0xb4

        // for players
        int weapon;      // 0xc4 determines weapon and flash model, etc
        int weaponModel; // 0xc8
        int legsAnim;    // 0xcc mask off ANIM_TOGGLEBIT
        int torsoAnim;   // 0xd0 mask off ANIM_TOGGLEBIT

        union
        {
            int helicopterStage; // 0xd4
        } un1;

        int un2;                  // 0xd8
        int fTorsoPitch;          // 0xdc
        int fWaistPitch;          // 0xe0
        unsigned int partBits[4]; // 0xe4
    };

    static_assert(sizeof(entityState_s) == 0xf4, "");
    static_assert(offsetof(entityState_s, index) == 0x88, "");

    struct turretInfo_s;
    struct scr_vehicle_s;

    struct item_ent_t
    {
        int ammoCount;
        int clipAmmoCount;
        int index;
    };

    struct __declspec(align(4)) trigger_ent_t
    {
        int threshold;
        int accumulate;
        int timestamp;
        int singleUserEntIndex;
        bool requireLookAt;
    };

    struct mover_ent_t
    {
        float decelTime;
        float aDecelTime;
        float speed;
        float aSpeed;
        float midTime;
        float aMidTime;
        float pos1[3];
        float pos2[3];
        float pos3[3];
        float apos1[3];
        float apos2[3];
        float apos3[3];
    };

    struct corpse_ent_t
    {
        int deathAnimStartTime;
    };

    enum MissileStage : __int32
    {
        MISSILESTAGE_SOFTLAUNCH = 0x0,
        MISSILESTAGE_ASCENT = 0x1,
        MISSILESTAGE_DESCENT = 0x2,
    };

    enum MissileFlightMode : __int32
    {
        MISSILEFLIGHTMODE_TOP = 0x0,
        MISSILEFLIGHTMODE_DIRECT = 0x1,
    };

    struct missile_ent_t
    {
        float time;
        int timeOfBirth;
        float travelDist;
        float surfaceNormal[3];
        team_t team;
        float curvature[3];
        float targetOffset[3];
        MissileStage stage;
        MissileFlightMode flightMode;
    };

    union $71462809E721C4770F13398DD6519E31
    {
        item_ent_t item[2];
        trigger_ent_t trigger;
        mover_ent_t mover;
        corpse_ent_t corpse;
        missile_ent_t missile;
    };

    struct tagInfo_s;

    struct gentity_s
    {
        entityState_s s;
        entityShared_t r;
        gclient_s *client;
        turretInfo_s *pTurretInfo;
        scr_vehicle_s *scr_vehicle;
        unsigned __int16 model;
        unsigned __int8 physicsObject;
        unsigned __int8 takedamage;
        unsigned __int8 active;
        unsigned __int8 nopickup;
        unsigned __int8 handler;
        unsigned __int8 team;
        unsigned __int16 classname;
        unsigned __int16 target;
        unsigned __int16 targetname;
        unsigned int attachIgnoreCollision;
        int spawnflags;
        int flags;
        int eventTime;
        int freeAfterEvent;
        int unlinkAfterEvent;
        int clipmask;
        int processedFrame;
        EntHandle parent;
        int nextthink;
        int health;
        int maxHealth;
        int damage;
        int count;
        gentity_s *chain;
        $71462809E721C4770F13398DD6519E31 ___u30;
        EntHandle missileTargetEnt;
        tagInfo_s *tagInfo;
        gentity_s *tagChildren;
        unsigned __int16 attachModelNames[19];
        unsigned __int16 attachTagNames[19];
        int useCount;
        gentity_s *nextFree;
    };

    static_assert(offsetof(gentity_s, client) == 0x0015C, "");
    static_assert(sizeof(gentity_s) == 0x274, ""); // retail TU4 size is 0x278 bytes. TODO: check padding

    struct cmd_function_s
    {
        cmd_function_s *next;
        const char *name;
        const char *autoCompleteDir;
        const char *autoCompleteExt;
        void(__fastcall *function)();
    };

    enum keyNum_t : __int32
    {
        K_NONE = 0x0,
        K_FIRSTGAMEPADBUTTON_RANGE_1 = 0x1,
        K_BUTTON_A = 0x1,
        K_BUTTON_B = 0x2,
        K_BUTTON_X = 0x3,
        K_BUTTON_Y = 0x4,
        K_BUTTON_LSHLDR = 0x5,
        K_BUTTON_RSHLDR = 0x6,
        K_LASTGAMEPADBUTTON_RANGE_1 = 0x6,
        K_TAB = 0x9,
        K_ENTER = 0xD,
        K_FIRSTGAMEPADBUTTON_RANGE_2 = 0xE,
        K_BUTTON_START = 0xE,
        K_BUTTON_BACK = 0xF,
        K_BUTTON_LSTICK = 0x10,
        K_BUTTON_RSTICK = 0x11,
        K_BUTTON_LTRIG = 0x12,
        K_BUTTON_RTRIG = 0x13,
        K_DPAD_UP = 0x14,
        K_FIRSTDPAD = 0x14,
        K_DPAD_DOWN = 0x15,
        K_DPAD_LEFT = 0x16,
        K_DPAD_RIGHT = 0x17,
        K_BUTTON_LSTICK_ALTIMAGE = 0xBC,
        K_BUTTON_RSTICK_ALTIMAGE = 0xBD,
        K_LASTDPAD = 0x17,
        K_LASTGAMEPADBUTTON_RANGE_2 = 0x17,
        K_ESCAPE = 0x1B,
        K_FIRSTGAMEPADBUTTON_RANGE_3 = 0x1C,
        K_APAD_UP = 0x1C,
        K_FIRSTAPAD = 0x1C,
        K_APAD_DOWN = 0x1D,
        K_APAD_LEFT = 0x1E,
        K_APAD_RIGHT = 0x1F,
        K_LASTAPAD = 0x1F,
        K_LASTGAMEPADBUTTON_RANGE_3 = 0x1F,
        K_SPACE = 0x20,
        K_BACKSPACE = 0x7F,
        K_ASCII_FIRST = 0x80,
        K_ASCII_181 = 0x80,
        K_ASCII_191 = 0x81,
        K_ASCII_223 = 0x82,
        K_ASCII_224 = 0x83,
        K_ASCII_225 = 0x84,
        K_ASCII_228 = 0x85,
        K_ASCII_229 = 0x86,
        K_ASCII_230 = 0x87,
        K_ASCII_231 = 0x88,
        K_ASCII_232 = 0x89,
        K_ASCII_233 = 0x8A,
        K_ASCII_236 = 0x8B,
        K_ASCII_241 = 0x8C,
        K_ASCII_242 = 0x8D,
        K_ASCII_243 = 0x8E,
        K_ASCII_246 = 0x8F,
        K_ASCII_248 = 0x90,
        K_ASCII_249 = 0x91,
        K_ASCII_250 = 0x92,
        K_ASCII_252 = 0x93,
        K_END_ASCII_CHARS = 0x94,
        K_COMMAND = 0x96,
        K_CAPSLOCK = 0x97,
        K_POWER = 0x98,
        K_PAUSE = 0x99,
        K_UPARROW = 0x9A,
        K_DOWNARROW = 0x9B,
        K_LEFTARROW = 0x9C,
        K_RIGHTARROW = 0x9D,
        K_ALT = 0x9E,
        K_CTRL = 0x9F,
        K_SHIFT = 0xA0,
        K_INS = 0xA1,
        K_DEL = 0xA2,
        K_PGDN = 0xA3,
        K_PGUP = 0xA4,
        K_HOME = 0xA5,
        K_END = 0xA6,
        K_F1 = 0xA7,
        K_F2 = 0xA8,
        K_F3 = 0xA9,
        K_F4 = 0xAA,
        K_F5 = 0xAB,
        K_F6 = 0xAC,
        K_F7 = 0xAD,
        K_F8 = 0xAE,
        K_F9 = 0xAF,
        K_F10 = 0xB0,
        K_F11 = 0xB1,
        K_F12 = 0xB2,
        K_F13 = 0xB3,
        K_F14 = 0xB4,
        K_F15 = 0xB5,
        K_KP_HOME = 0xB6,
        K_KP_UPARROW = 0xB7,
        K_KP_PGUP = 0xB8,
        K_KP_LEFTARROW = 0xB9,
        K_KP_5 = 0xBA,
        K_KP_RIGHTARROW = 0xBB,
        K_KP_END = 0xBC,
        K_KP_DOWNARROW = 0xBD,
        K_KP_PGDN = 0xBE,
        K_KP_ENTER = 0xBF,
        K_KP_INS = 0xC0,
        K_KP_DEL = 0xC1,
        K_KP_SLASH = 0xC2,
        K_KP_MINUS = 0xC3,
        K_KP_PLUS = 0xC4,
        K_KP_NUMLOCK = 0xC5,
        K_KP_STAR = 0xC6,
        K_KP_EQUALS = 0xC7,
        K_MOUSE1 = 0xC8,
        K_MOUSE2 = 0xC9,
        K_MOUSE3 = 0xCA,
        K_MOUSE4 = 0xCB,
        K_MOUSE5 = 0xCC,
        K_MWHEELDOWN = 0xCD,
        K_MWHEELUP = 0xCE,
        K_AUX1 = 0xCF,
        K_AUX2 = 0xD0,
        K_AUX3 = 0xD1,
        K_AUX4 = 0xD2,
        K_AUX5 = 0xD3,
        K_AUX6 = 0xD4,
        K_AUX7 = 0xD5,
        K_AUX8 = 0xD6,
        K_AUX9 = 0xD7,
        K_AUX10 = 0xD8,
        K_AUX11 = 0xD9,
        K_AUX12 = 0xDA,
        K_AUX13 = 0xDB,
        K_AUX14 = 0xDC,
        K_AUX15 = 0xDD,
        K_AUX16 = 0xDE,
        K_LAST_KEY = 0xDF,
    };

    struct MapEnts
    {
        const char *name;
        char *entityString;
        int numEntityChars;
    };

    enum XAssetType : __int32
    {
        ASSET_TYPE_XMODELPIECES = 0x0,
        ASSET_TYPE_PHYSPRESET = 0x1,
        ASSET_TYPE_XANIMPARTS = 0x2,
        ASSET_TYPE_XMODEL = 0x3,
        ASSET_TYPE_MATERIAL = 0x4,
        ASSET_TYPE_PIXELSHADER = 0x5,
        ASSET_TYPE_TECHNIQUE_SET = 0x6,
        ASSET_TYPE_IMAGE = 0x7,
        ASSET_TYPE_SOUND = 0x8,
        ASSET_TYPE_SOUND_CURVE = 0x9,
        ASSET_TYPE_LOADED_SOUND = 0xA,
        ASSET_TYPE_CLIPMAP = 0xB,
        ASSET_TYPE_CLIPMAP_PVS = 0xC,
        ASSET_TYPE_COMWORLD = 0xD,
        ASSET_TYPE_GAMEWORLD_SP = 0xE,
        ASSET_TYPE_GAMEWORLD_MP = 0xF,
        ASSET_TYPE_MAP_ENTS = 0x10,
        ASSET_TYPE_GFXWORLD = 0x11,
        ASSET_TYPE_LIGHT_DEF = 0x12,
        ASSET_TYPE_UI_MAP = 0x13,
        ASSET_TYPE_FONT = 0x14,
        ASSET_TYPE_MENULIST = 0x15,
        ASSET_TYPE_MENU = 0x16,
        ASSET_TYPE_LOCALIZE_ENTRY = 0x17,
        ASSET_TYPE_WEAPON = 0x18,
        ASSET_TYPE_SNDDRIVER_GLOBALS = 0x19,
        ASSET_TYPE_FX = 0x1A,
        ASSET_TYPE_IMPACT_FX = 0x1B,
        ASSET_TYPE_AITYPE = 0x1C,
        ASSET_TYPE_MPTYPE = 0x1D,
        ASSET_TYPE_CHARACTER = 0x1E,
        ASSET_TYPE_XMODELALIAS = 0x1F,
        ASSET_TYPE_RAWFILE = 0x20,
        ASSET_TYPE_STRINGTABLE = 0x21,
        ASSET_TYPE_COUNT = 0x22,
        ASSET_TYPE_STRING = 0x22,
        ASSET_TYPE_ASSETLIST = 0x23,
    };

    /* 760 */
    enum MapType : __int32
    {
        MAPTYPE_NONE = 0x0,
        MAPTYPE_INVALID1 = 0x1,
        MAPTYPE_INVALID2 = 0x2,
        MAPTYPE_2D = 0x3,
        MAPTYPE_3D = 0x4,
        MAPTYPE_CUBE = 0x5,
        MAPTYPE_COUNT = 0x6,
    };

    /* 8713 */
    union GfxTexture
    {
        D3DBaseTexture *basemap;
        D3DTexture *map;
        D3DVolumeTexture *volmap;
        D3DCubeTexture *cubemap;
        struct GfxImageLoadDef *loadDef; // forward declaration
    };

    /* 8714 */
    struct GfxImageLoadDef
    {
        unsigned __int8 levelCount;
        unsigned __int8 flags;
        __int16 dimensions[3];
        int format;
        GfxTexture texture;
    };

    /* 8715 */
    struct CardMemory
    {
        int platform[1];
    };

    /* 8716 */
    struct GfxImage
    {
        MapType mapType;
        GfxTexture texture;
        unsigned __int8 semantic;
        CardMemory cardMemory;
        unsigned __int16 width;
        unsigned __int16 height;
        unsigned __int16 depth;
        unsigned __int8 category;
        bool delayLoadPixels;
        unsigned __int8 *pixels;
        unsigned int baseSize;
        unsigned __int16 streamSlot;
        bool streaming;
        const char *name;
    };

    struct RawFile
    {
        const char *name;
        int len;
        const char *buffer;
    };

    struct Material;

    struct Glyph
    {
        unsigned __int16 letter;
        char x0;
        char y0;
        unsigned __int8 dx;
        unsigned __int8 pixelWidth;
        unsigned __int8 pixelHeight;
        float s0;
        float t0;
        float s1;
        float t1;
    };
    static_assert(sizeof(Glyph) == 0x18, "");

    struct Font_s
    {
        const char *fontName;
        int pixelHeight;
        int glyphCount;
        Material *material;
        Material *glowMaterial;
        Glyph *glyphs;
    };
    static_assert(sizeof(Font_s) == 0x18, "");

    union XAssetHeader
    {
        // XModelPieces *xmodelPieces;
        // PhysPreset *physPreset;
        // XAnimParts *parts;
        // XModel *model;
        Material *material;
        // MaterialPixelShader *pixelShader;
        // MaterialVertexShader *vertexShader;
        // MaterialTechniqueSet *techniqueSet;
        GfxImage *image;
        // snd_alias_list_t *sound;
        // SndCurve *sndCurve;
        // LoadedSound *loadSnd;
        // clipMap_t *clipMap;
        // ComWorld *comWorld;
        // GameWorldSp *gameWorldSp;
        // GameWorldMp *gameWorldMp;
        // MapEnts *mapEnts;
        // GfxWorld *gfxWorld;
        // GfxLightDef *lightDef;
        Font_s *font;
        // MenuList *menuList;
        // menuDef_t *menu;
        // LocalizeEntry *localize;
        // WeaponDef *weapon;
        // SndDriverGlobals *sndDriverGlobals;
        // const FxEffectDef *fx;
        // FxImpactTable *impactFx;
        RawFile *rawfile;
        // StringTable *stringTable;
        void *data;
    };

    enum conChannel_t : __int32
    {
        CON_CHANNEL_DONT_FILTER = 0x0,
        CON_CHANNEL_ERROR = 0x1,
        CON_CHANNEL_GAMENOTIFY = 0x2,
        CON_CHANNEL_BOLDGAME = 0x3,
        CON_CHANNEL_SUBTITLE = 0x4,
        CON_CHANNEL_OBITUARY = 0x5,
        CON_CHANNEL_LOGFILEONLY = 0x6,
        CON_CHANNEL_CONSOLEONLY = 0x7,
        CON_CHANNEL_GFX = 0x8,
        CON_CHANNEL_SOUND = 0x9,
        CON_CHANNEL_FILES = 0xA,
        CON_CHANNEL_DEVGUI = 0xB,
        CON_CHANNEL_PROFILE = 0xC,
        CON_CHANNEL_UI = 0xD,
        CON_CHANNEL_CLIENT = 0xE,
        CON_CHANNEL_SERVER = 0xF,
        CON_CHANNEL_SYSTEM = 0x10,
        CON_CHANNEL_PLAYERWEAP = 0x11,
        CON_CHANNEL_AI = 0x12,
        CON_CHANNEL_ANIM = 0x13,
        CON_CHANNEL_PHYS = 0x14,
        CON_CHANNEL_FX = 0x15,
        CON_CHANNEL_LEADERBOARDS = 0x16,
        CON_CHANNEL_PARSERSCRIPT = 0x17,
        CON_CHANNEL_SCRIPT = 0x18,
        CON_BUILTIN_CHANNEL_COUNT = 0x19,
    };

    struct ImageList
    {
        unsigned int count;
        GfxImage *image[2048];
    };

    struct XAsset
    {
        XAssetType type;
        XAssetHeader header;
    };

    struct XAssetEntry
    {
        XAsset asset;
        unsigned __int8 zoneIndex;
        bool inuse;
        unsigned __int16 nextHash;
        unsigned __int16 nextOverride;
        unsigned __int16 usageFrame;
    };

    union XAssetEntryPoolEntry
    {
        XAssetEntry entry;
        XAssetEntryPoolEntry *next;
    };

    enum svscmd_type : __int32
    {
        SV_CMD_CAN_IGNORE = 0x0,
        SV_CMD_RELIABLE = 0x1,
    };

    struct client_t;

    union DvarValue
    {
        bool enabled;
        int integer;
        unsigned int unsignedInt;
        float value;
        float vector[4];
        const char *string;
        unsigned __int8 color[4];
    };

    union DvarLimits
    {
        struct
        {
            int stringCount;
            const char **strings;
        } enumeration;

        struct
        {
            int min;
            int max;
        } integer;

        struct
        {
            float min;
            float max;
        } value;

        struct
        {
            float min;
            float max;
        } vector;
    };

    struct dvar_s
    {
        const char *name;
        const char *description;
        unsigned __int16 flags;
        unsigned __int8 type;
        bool modified;
        DvarValue current;
        DvarValue latched;
        DvarValue reset;
        DvarLimits domain;
        dvar_s *hashNext;
    };

    struct ScreenPlacement
    {
        float scaleVirtualToReal[2];
        float scaleVirtualToFull[2];
        float scaleRealToVirtual[2];
        float virtualViewableMin[2];
        float virtualViewableMax[2];
        float realViewportSize[2];
        float realViewableMin[2];
        float realViewableMax[2];
        float subScreenLeft;
    };

    struct GfxViewInfo;

    struct pmove_t;

    struct viewState_t;

    struct weaponState_t;

    enum connstate_t
    {
        CA_DISCONNECTED = 0x0,
        CA_CINEMATIC = 0x1,
        CA_LOGO = 0x2,
        CA_CONNECTING = 0x3,
        CA_CHALLENGING = 0x4,
        CA_CONNECTED = 0x5,
        CA_SENDINGSTATS = 0x6,
        CA_LOADING = 0x7,
        CA_PRIMED = 0x8,
        CA_ACTIVE = 0x9,
    };

    struct clientUIActive_t
    {
        bool active;
        bool isRunning;
        bool cgameInitialized;
        bool cgameInitCalled;
        bool mapPreloaded;
        int keyCatchers;
        bool displayHUDWithKeycatchUI;
        connstate_t connectionState;
        int nextScrollTime;
        bool invited;
        int numVoicePacketsSent;
        int numVoicePacketsSentStart;
    };

    typedef void (*BG_CalculateView_IdleAngles_t)(viewState_t *vs, float *angles);
    typedef void (*BG_CalculateWeaponPosition_IdleAngles_t)(weaponState_t *ws, float *angles);

    typedef void (*Cbuf_AddText_t)(int localClientNum, const char *text);
    typedef void (*Cbuf_ExecuteBuffer_t)(int localClientNum, int controllerIndex, const char *buffer);

    typedef void (*CG_DrawActive_t)(int localClientNum);
    typedef void (*CG_GameMessage_t)(int localClientNum, const char *msg);
    typedef const playerState_s *(*CG_GetPredictedPlayerState_t)(int localClientNum);
    typedef void (*CG_RegisterGraphics_t)(int localClientNum, const char *mapname);

    typedef void (*CL_ConsolePrint_t)(int localClientNum, int channel, const char *txt, int duration, int pixelWidth, int flags);
    typedef void (*CL_GamepadButtonEvent_t)(int localClientNum, int controllerIndex, int key, int down, unsigned int time);

    typedef void (*ClientCommand_t)(int clientNum);

    typedef void (*Cmd_AddCommandInternal_t)(const char *cmdName, void (*function)(), cmd_function_s *allocedCmd);
    typedef bool (*Cmd_ExecFromFastFile_t)(int localClientNum, int controllerIndex, const char *filename);
    typedef void (*Cmd_ExecuteSingleCommand_t)(int localClientNum, int controllerIndex, const char *text);

    typedef int (*CheatsOk_t)(gentity_s *ent);

    typedef void (*Com_PrintError_t)(conChannel_t channel, const char *fmt, ...);
    typedef void (*Com_PrintMessage_t)(conChannel_t channel, const char *msg, int error);
    typedef void (*Com_Printf_t)(conChannel_t channel, const char *fmt, ...);
    typedef void (*Com_PrintWarning_t)(conChannel_t channel, const char *fmt, ...);

    typedef void (*DB_EnumXAssets_FastFile_t)(XAssetType type, void (*func)(void *asset, void *inData), void *inData, bool includeOverride);
    typedef XAssetEntryPoolEntry *(*DB_FindXAssetEntry_t)(XAssetType type, const char *name);
    typedef XAssetHeader *(*DB_FindXAssetHeader_t)(const XAssetType type, const char *name);
    typedef int (*DB_GetAllXAssetOfType_FastFile_t)(XAssetType type, XAssetHeader *assets, int maxCount);

    typedef bool (*Dvar_GetBool_t)(const char *dvarName);
    typedef dvar_s *(*Dvar_RegisterBool_t)(const char *dvarName, bool value, unsigned __int16 flags, const char *description);
    typedef dvar_s *(*Dvar_RegisterColor_t)(const char *dvarName, double r, double g, double b, double a, unsigned __int16 flags, const char *description);
    typedef dvar_s *(*Dvar_RegisterEnum_t)(const char *dvarName, const char **valueList, unsigned __int16 defaultIndex, unsigned __int16 flags, const char *description);
    typedef dvar_s *(*Dvar_RegisterInt_t)(const char *dvarName, int value, int min, int max, unsigned __int16 flags, const char *description);

    typedef void (*G_SetAngle_t)(gentity_s *ent, float *origin);
    typedef void (*G_SetOrigin_t)(gentity_s *ent, float *origin);

    typedef int (*I_strnicmp_t)(const char *s0, const char *s1, int n);

    typedef void (*Load_MapEntsPtr_t)();

    typedef void (*PM_FoliageSounds_t)(pmove_t *pm);

    typedef void (*R_AddCmdDrawText_t)(const char *text, int maxChars, Font_s *font, double x, double y, double xScale, double yScale, double rotation, const float *color, int style);
    typedef void (*R_DrawAllDynEnt_t)(const GfxViewInfo *viewInfo);
    typedef void (*R_GetImageList_t)(ImageList *imageList);
    typedef int (*R_RegisterFont_t)(const char *name);
    typedef int (*R_StreamLoadFileSynchronously_t)(const char *filename, unsigned int bytesToRead, unsigned __int8 *outData);

    typedef void (*SetClientViewAngle_t)(gentity_s *ent, float *angle);

    typedef void (*SCR_DrawSmallStringExt_t)(unsigned int x, unsigned int y, const char *string, const float *setColor);

    typedef char *(*Scr_ReadFile_FastFile_t)(const char *filename, const char *extFilename, const char *codePos, bool archive);

    typedef void (*SV_Cmd_ArgvBuffer_t)(int arg, char *buffer, int bufferLength);
    typedef void (*SV_GameSendServerCommand_t)(int clientNum, svscmd_type type, const char *text);
    typedef void (*SV_SendServerCommand_t)(client_t *cl, svscmd_type type, const char *fmt, ...);

    typedef void (*Sys_SnapVector_t)(float *result);

    typedef void (*TeleportPlayer_t)(gentity_s *player, float *origin, float *angles);

    typedef void (*UI_DrawBuildNumber_t)(const int localClientNum);
    typedef void (*UI_DrawText_t)(const ScreenPlacement *scrPlace, const char *text, int maxChars, Font_s *font, double x, double y, int horzAlign, int vertAlign, double scale, const float *color, int style);
    typedef void (*UI_Refresh_t)(int localClientNum);

    typedef char *(*va_t)(char *format, ...);

    // Custom additions
    enum pm_fps_mode_t
    {
        PM_FPS_MODE_CONSOLE,
        PM_FPS_MODE_PC
    };
}

#pragma warning(default : 4480)
