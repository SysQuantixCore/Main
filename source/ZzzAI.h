#pragma once

// Dependências: vec3_t, OBJECT, CHARACTER, PATH_t, BYTE, TW_CHARACTER
// Inclua antes deste header os arquivos que definem esses tipos.

// ---------------------------------------------------------------------------
// Globais (definidas em ZzzAI.cpp)
// ---------------------------------------------------------------------------
extern float  FPS;
extern double DeltaT;
extern float  WorldTime;

// ---------------------------------------------------------------------------
// Ângulos (graus, 0 ~ 360)
// ---------------------------------------------------------------------------
int   CalcAngle(float PositionX, float PositionY, float TargetX, float TargetY);
float CreateAngle(float x1, float y1, float x2, float y2);
int   TurnAngle(int iTheta, int iHeading, int maxTURN);
float TurnAngle2(float angle, float a, float d);
float FarAngle(float angle1, float angle2, bool bAbs = true);

// ---------------------------------------------------------------------------
// Movimento
// ---------------------------------------------------------------------------
float MoveHumming(vec3_t Position, vec3_t Angle, vec3_t TargetPosition, float fTurn, bool bTimeFactor = true);
void  MovePosition(vec3_t Position, vec3_t Angle, vec3_t Speed, bool bTimeFactor = true);
BYTE  CalcTargetPos(float x, float y, int Tx, int Ty);
void  MoveBoid(OBJECT* o, int i, OBJECT* Boids, int MAX);
void  PushObject(vec3_t PushPosition, vec3_t Position, float Power, vec3_t Angle);
void  Alpha(OBJECT* o);

// ---------------------------------------------------------------------------
// Colisão (definidas em outro arquivo; confirme antes de usar)
// ---------------------------------------------------------------------------
bool CollisionCharacters(CHARACTER* Hero);
bool CollisionObjects(CHARACTER* Hero);

// ---------------------------------------------------------------------------
// Ação / teste
// ---------------------------------------------------------------------------
void SetAction(OBJECT* o, int Action, bool bBlending = true);
bool TestDistance(CHARACTER* c, vec3_t TargetPosition, float Range);
void LookAtTarget(OBJECT* o, CHARACTER* tc);
void MoveHead(CHARACTER* c);

// ---------------------------------------------------------------------------
// Combate (CalculateAttackPoint, AttackCharacter e AttackObject
// não são definidas em ZzzAI.cpp; confirme onde estão implementadas)
// ---------------------------------------------------------------------------
void Damage(vec3_t soPosition, CHARACTER* tc, float AttackRange, int AttackPoint, bool Hit);
int  CalculateAttackPoint(CHARACTER* sc, CHARACTER* tc);
void AttackCharacter(CHARACTER* sc, CHARACTER* tc, float AttackRange, int AttackPoint, bool bSuccess = true);
void AttackObject(int Type, vec3_t Position, CHARACTER* Owner);

// ---------------------------------------------------------------------------
// Caminho
// ---------------------------------------------------------------------------
void InitPath();
bool MovePath(CHARACTER* c, bool bTurn = true);
bool PathFinding2(int sx, int sy, int tx, int ty, PATH_t* a, float fDistance = 0.0f, int iDefaultWall = TW_CHARACTER);

// ---------------------------------------------------------------------------
// Fenrir / Alice
// ---------------------------------------------------------------------------
void SetAction_Fenrir_Skill(CHARACTER* c, OBJECT* o);
void SetAction_Fenrir_Damage(CHARACTER* c, OBJECT* o);
void SetAction_Fenrir_Run(CHARACTER* c, OBJECT* o);
void SetAction_Fenrir_Walk(CHARACTER* c, OBJECT* o);

bool IsAliceRideAction_UniDino(unsigned short byAction);
bool IsAliceRideAction_Fenrir(unsigned short byAction);