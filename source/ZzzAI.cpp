///////////////////////////////////////////////////////////////////////////////
// ZzzAI.cpp
// Funções de IA: rotação em direção ao alvo, movimento, busca de caminho,
// cálculo de FPS, ações do Fenrir etc.
//
// *** Função de nível: 2
//
// Salve o arquivo em UTF-8 com BOM para que os comentários sejam lidos
// corretamente pelo compilador.
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ZzzBMD.h"
#include "ZzzInfomation.h"
#include "ZzzObject.h"
#include "ZzzCharacter.h"
#include "ZzzLodTerrain.h"
#include "ZzzAI.h"
#include "ZzzTexture.h"
#include "ZzzOpenglUtil.h"
#include "ZzzInterface.h"
#include "ZzzEffect.h"
#include "zzzScene.h"
#include "DSPlaySound.h"
#include "GMCrywolf1st.h"
#include "ZzzPath.h"
#include "CharacterManager.h"
#include "SkillManager.h"

// ---------------------------------------------------------------------------
// Globais
// ---------------------------------------------------------------------------
float  FPS;
double DeltaT = 0.1f;
float  WorldTime = 0.f;

// ---------------------------------------------------------------------------
// Ângulos
// ---------------------------------------------------------------------------

// Ângulo do vetor (x1,y1) -> (x2,y2), em graus, 0 a 360.
float CreateAngle(float x1, float y1, float x2, float y2)
{
	const float nx2 = x2 - x1;
	const float ny2 = y2 - y1;

	if (absf(nx2) < 0.0001f)
		return (ny2 < 0.f) ? 0.f : 180.f;

	if (absf(ny2) < 0.0001f)
		return (nx2 < 0.f) ? 270.f : 90.f;

	float angle = (float)atan(ny2 / nx2) / 3.1415926536f * 180.f + 90.f;
	if (nx2 < 0.f)
		angle += 180.f;

	return angle;
}

// Gira iTheta em direção a iHeading, no máximo maxTURN graus por chamada.
int TurnAngle(int iTheta, int iHeading, int maxTURN)
{
	int iChange = 0;
	const int Delta = abs(iTheta - iHeading);

	if (iTheta > iHeading)
	{
		if (Delta < abs((iHeading + 360) - iTheta))
			iChange = -min(maxTURN, Delta);
		else
			iChange = min(maxTURN, Delta);
	}
	else if (iTheta < iHeading)
	{
		if (Delta < abs((iTheta + 360) - iHeading))
			iChange = min(maxTURN, Delta);
		else
			iChange = -min(maxTURN, Delta);
	}

	iTheta += iChange + 360;
	iTheta %= 360;
	return iTheta;
}

// Gira 'angle' em direção a 'a' em no máximo 'd' graus.
float TurnAngle2(float angle, float a, float d)
{
	if (angle < 0.f)
		angle += 360.f;
	if (a < 0.f)
		a += 360.f;

	if (angle < 180.f)
	{
		const float aa = angle - d;

		if (a >= angle + d && a < angle + 180.f)
			angle += d;
		else if (aa >= 0.f && (a >= angle + 180.f || a < aa))
			angle -= d;
		else if (aa < 0.f && (a >= angle + 180.f && a < aa + 360.f))
			angle = angle - d + 360.f;
		else
			angle = a;
	}
	else
	{
		const float aa = angle + d;

		if (a < angle - d && a >= angle - 180.f)
			angle -= d;
		else if (aa < 360.f && (a < angle - 180.f || a >= aa))
			angle += d;
		else if (aa >= 360.f && (a < angle - 180.f && a >= aa - 360.f))
			angle = angle + d - 360.f;
		else
			angle = a;
	}
	return angle;
}

// Diferença angular entre dois ângulos (considera o caminho mais curto).
float FarAngle(float angle1, float angle2, bool bAbs)
{
	if (angle1 < 0.f)
		angle1 += 360.f;
	if (angle2 < 0.f)
		angle2 += 360.f;

	float d = angle2 - angle1;

	if (angle1 < 180.f)
	{
		if (angle2 >= angle1 + 180.f)
			d = 360.f - angle2 + angle1;
	}
	else
	{
		if (angle2 < angle1 - 180.f)
			d = 360.f - angle1 + angle2;
	}

	if (bAbs && d < 0.f)
		d = -d;

	return d;
}

// Ângulo inteiro de (PositionX,PositionY) para (TargetX,TargetY), em graus.
int CalcAngle(float PositionX, float PositionY, float TargetX, float TargetY)
{
	// atan2f trata também o caso TargetX == PositionX (linha vertical).
	int theta = (int)(atan2f(PositionY - TargetY, TargetX - PositionX) * (180.f / Q_PI));
	if (theta < 0)
		theta += 360;

	return (360 - theta) % 360;
}

// ---------------------------------------------------------------------------
// Movimento
// ---------------------------------------------------------------------------

// Gira Angle em direção ao alvo e retorna a distância 3D até o alvo.
float MoveHumming(vec3_t Position, vec3_t Angle, vec3_t TargetPosition, float fTurn, bool bTimeFactor)
{
	vec3_t Range;
	const float fStep = bTimeFactor ? timefac(fTurn) : fTurn;

	float TargetAngle = CreateAngle(Position[0], Position[1], TargetPosition[0], TargetPosition[1]);
	Angle[2] = TurnAngle2(Angle[2], TargetAngle, fStep);

	VectorSubtract(Position, TargetPosition, Range);

	const float Distance = sqrtf(Range[0] * Range[0] + Range[1] * Range[1]);
	TargetAngle = 360.f - CreateAngle(Position[2], Distance, TargetPosition[2], 0.f);
	Angle[0] = TurnAngle2(Angle[0], TargetAngle, fStep);

	return VectorLength(Range);
}

// Move Position na direção de Angle, com velocidade Speed (no referencial local).
void MovePosition(vec3_t Position, vec3_t Angle, vec3_t Speed, bool bTimeFactor)
{
	float Matrix[3][4];
	AngleMatrix(Angle, Matrix);

	vec3_t Velocity;
	VectorRotate(Speed, Matrix, Velocity);

	for (int i = 0; i < 3; i++)
		Position[i] += bTimeFactor ? timefac(Velocity[i]) : Velocity[i];
}

// Empacota a posição relativa do alvo em 1 byte (2 nibbles de 0 a 15, centro em 8).
BYTE CalcTargetPos(float x, float y, int Tx, int Ty)
{
	const int PositionX = (int)(x / TERRAIN_SCALE);
	const int PositionY = (int)(y / TERRAIN_SCALE);
	const int TargetX = (int)(Tx / TERRAIN_SCALE);
	const int TargetY = (int)(Ty / TERRAIN_SCALE);

	int dx = 8 + TargetX - PositionX;
	int dy = 8 + TargetY - PositionY;

	// Cada delta precisa caber em 4 bits. Fora disso, os bits vazavam para o outro nibble.
	dx = max(0, min(dx, 15));
	dy = max(0, min(dy, 15));

	return (BYTE)((dx & 0x0F) | ((dy & 0x0F) << 4));
}

// Atualiza o alpha do objeto em direção a AlphaTarget.
void Alpha(OBJECT* o)
{
	const float kStep = 0.05f;

	if (o->AlphaEnable)
	{
		if (o->AlphaTarget > o->Alpha)
			o->Alpha = min(o->Alpha + kStep, 1.f);
		else if (o->AlphaTarget < o->Alpha)
			o->Alpha = max(o->Alpha - kStep, 0.f);
	}
	else
	{
		o->Alpha += (o->AlphaTarget - o->Alpha) * 0.1f;
	}

	if (o->BlendMeshLight > o->Alpha)
		o->BlendMeshLight = o->Alpha;
}

// Comportamento de bando (boids): separação, alinhamento e coesão simplificados.
void MoveBoid(OBJECT* o, int i, OBJECT* Boids, int MAX)
{
	int   NumBirds = 0;
	float TargetX = 0.f;
	float TargetY = 0.f;

	for (int j = 0; j < MAX; j++)
	{
		OBJECT* t = &Boids[j];
		if (!t->Live || j == i)
			continue;

		vec3_t Range;
		VectorSubtract(o->Position, t->Position, Range);
		const float Distance = sqrtf(Range[0] * Range[0] + Range[1] * Range[1]);
		if (Distance >= 400.f)
			continue;

		float xdist = t->Direction[0] - t->Position[0];
		float ydist = t->Direction[1] - t->Position[1];

		if (Distance < 80.f)
		{
			xdist -= t->Direction[0] - o->Position[0];
			ydist -= t->Direction[1] - o->Position[1];
		}
		else
		{
			xdist += t->Direction[0] - o->Position[0];
			ydist += t->Direction[1] - o->Position[1];
		}

		const float pdist = sqrtf(xdist * xdist + ydist * ydist);
		if (pdist < 0.0001f)
			continue;

		TargetX += xdist / pdist;
		TargetY += ydist / pdist;
		NumBirds++;
	}

	if (NumBirds > 0)
	{
		TargetX = o->Position[0] + TargetX / NumBirds;
		TargetY = o->Position[1] + TargetY / NumBirds;

		o->Angle[2] = (float)TurnAngle((int)o->Angle[2],
			CalcAngle(o->Position[0], o->Position[1], TargetX, TargetY),
			(int)o->Gravity);
	}
}

// Empurra Position a partir de PushPosition e ajusta a altura ao terreno.
void PushObject(vec3_t PushPosition, vec3_t Position, float Power, vec3_t Angle)
{
	Vector(0.f, 0.f, 0.f, Angle);
	Angle[2] = CreateAngle(PushPosition[0], PushPosition[1], Position[0], Position[1]) + 180.f;
	if (Angle[2] >= 360.f)
		Angle[2] -= 360.f;

	float Matrix[3][4];
	AngleMatrix(Angle, Matrix);

	vec3_t p1, p2;
	Vector(0.f, -Power, 0.f, p1);
	VectorRotate(p1, Matrix, p2);

	Position[0] += p2[0];
	Position[1] += p2[1];
	Position[2] = RequestTerrainHeight(Position[0], Position[1]);
}

// ---------------------------------------------------------------------------
// Seleção de ação por armas (Fenrir)
// ---------------------------------------------------------------------------

// Escolhe a ação conforme as armas: duas, só direita, só esquerda ou nenhuma.
static int SelectActionByWeapon(const CHARACTER* c, int nTwo, int nRight, int nLeft, int nNone)
{
	const bool bRight = c->Weapon[0].Type != -1;
	const bool bLeft = c->Weapon[1].Type != -1;

	if (bRight && bLeft) return nTwo;
	if (bRight)          return nRight;
	if (bLeft)           return nLeft;
	return nNone;
}

// Igual à anterior, mas trata arco: arco no slot direito (MODEL_BOW+7) conta como
// arma única direita; arco no slot esquerdo (MODEL_BOW+15) conta como arma única esquerda.
static int SelectActionByWeaponWithBow(const CHARACTER* c, int nTwo, int nRight, int nLeft, int nNone)
{
	if (c->Weapon[0].Type != -1 && c->Weapon[1].Type != -1)
	{
		if (c->Weapon[1].Type == MODEL_BOW + 7)  return nRight;
		if (c->Weapon[0].Type == MODEL_BOW + 15) return nLeft;
		return nTwo;
	}
	return SelectActionByWeapon(c, nTwo, nRight, nLeft, nNone);
}

// ---------------------------------------------------------------------------
// Fenrir / Alice
// ---------------------------------------------------------------------------

void SetAction_Fenrir_Skill(CHARACTER* c, OBJECT* o)
{
#ifdef PBG_ADD_NEWCHAR_MONK_ANI
	if (gCharacterManager.GetBaseClass(c->Class) == CLASS_RAGEFIGHTER)
	{
		SetAction(o, SelectActionByWeapon(c,
			PLAYER_RAGE_FENRIR_TWO_SWORD,
			PLAYER_RAGE_FENRIR_ONE_RIGHT,
			PLAYER_RAGE_FENRIR_ONE_LEFT,
			PLAYER_RAGE_FENRIR));
		return;
	}
#endif //PBG_ADD_NEWCHAR_MONK_ANI

	SetAction(o, SelectActionByWeapon(c,
		PLAYER_FENRIR_SKILL_TWO_SWORD,
		PLAYER_FENRIR_SKILL_ONE_RIGHT,
		PLAYER_FENRIR_SKILL_ONE_LEFT,
		PLAYER_FENRIR_SKILL));
}

void SetAction_Fenrir_Damage(CHARACTER* c, OBJECT* o)
{
#ifdef PBG_ADD_NEWCHAR_MONK_ANI
	if (gCharacterManager.GetBaseClass(c->Class) == CLASS_RAGEFIGHTER)
	{
		SetAction(o, SelectActionByWeapon(c,
			PLAYER_RAGE_FENRIR_DAMAGE_TWO_SWORD,
			PLAYER_RAGE_FENRIR_DAMAGE_ONE_RIGHT,
			PLAYER_RAGE_FENRIR_DAMAGE_ONE_LEFT,
			PLAYER_RAGE_FENRIR_DAMAGE));
		return;
	}
#endif //PBG_ADD_NEWCHAR_MONK_ANI

	SetAction(o, SelectActionByWeaponWithBow(c,
		PLAYER_FENRIR_DAMAGE_TWO_SWORD,
		PLAYER_FENRIR_DAMAGE_ONE_RIGHT,
		PLAYER_FENRIR_DAMAGE_ONE_LEFT,
		PLAYER_FENRIR_DAMAGE));
}

void SetAction_Fenrir_Run(CHARACTER* c, OBJECT* o)
{
	const bool bRight = c->Weapon[0].Type != -1;
	const bool bLeft = c->Weapon[1].Type != -1;
	const int  nClass = c->GetBaseClass();

	// Duas armas (com tratamento de arco)
	if (bRight && bLeft)
	{
		if (c->Weapon[1].Type == MODEL_BOW + 7)
		{
			SetAction(o, PLAYER_FENRIR_RUN_ONE_RIGHT_ELF);
			return;
		}
		if (c->Weapon[0].Type == MODEL_BOW + 15)
		{
			SetAction(o, PLAYER_FENRIR_RUN_ONE_LEFT_ELF);
			return;
		}

		if (nClass == CLASS_ELF)
			SetAction(o, PLAYER_FENRIR_RUN_TWO_SWORD_ELF);
		else if (nClass == CLASS_DARK)
			SetAction(o, PLAYER_FENRIR_RUN_TWO_SWORD_MAGOM);
#ifdef PBG_ADD_NEWCHAR_MONK_ANI
		else if (nClass == CLASS_RAGEFIGHTER)
			SetAction(o, PLAYER_RAGE_FENRIR_RUN_TWO_SWORD);
#endif //PBG_ADD_NEWCHAR_MONK_ANI
		else
			SetAction(o, PLAYER_FENRIR_RUN_TWO_SWORD);
		return;
	}

	// Só arma direita
	if (bRight)
	{
		if (nClass == CLASS_ELF)
			SetAction(o, PLAYER_FENRIR_RUN_ONE_RIGHT_ELF);
		else if (nClass == CLASS_DARK)
			SetAction(o, PLAYER_FENRIR_RUN_ONE_RIGHT_MAGOM);
#ifdef PBG_ADD_NEWCHAR_MONK_ANI
		else if (nClass == CLASS_RAGEFIGHTER)
			SetAction(o, PLAYER_RAGE_FENRIR_RUN_ONE_RIGHT);
#endif //PBG_ADD_NEWCHAR_MONK_ANI
		else
			SetAction(o, PLAYER_FENRIR_RUN_ONE_RIGHT);
		return;
	}

	// Só arma esquerda
	if (bLeft)
	{
		if (nClass == CLASS_ELF)
			SetAction(o, PLAYER_FENRIR_RUN_ONE_LEFT_ELF);
		else if (nClass == CLASS_DARK)
			SetAction(o, PLAYER_FENRIR_RUN_ONE_LEFT_MAGOM);
#ifdef PBG_ADD_NEWCHAR_MONK_ANI
		else if (nClass == CLASS_RAGEFIGHTER)
			SetAction(o, PLAYER_RAGE_FENRIR_RUN_ONE_LEFT);
#endif //PBG_ADD_NEWCHAR_MONK_ANI
		else
			SetAction(o, PLAYER_FENRIR_RUN_ONE_LEFT);
		return;
	}

	// Sem armas
	if (nClass == CLASS_ELF)
		SetAction(o, PLAYER_FENRIR_RUN_ELF);
	else if (nClass == CLASS_DARK)
		SetAction(o, PLAYER_FENRIR_RUN_MAGOM);
#ifdef PBG_ADD_NEWCHAR_MONK_ANI
	else if (nClass == CLASS_RAGEFIGHTER)
		SetAction(o, PLAYER_RAGE_FENRIR_RUN);
#endif //PBG_ADD_NEWCHAR_MONK_ANI
	else
		SetAction(o, PLAYER_FENRIR_RUN);
}

void SetAction_Fenrir_Walk(CHARACTER* c, OBJECT* o)
{
#ifdef PBG_ADD_NEWCHAR_MONK_ANI
	if (gCharacterManager.GetBaseClass(c->Class) == CLASS_RAGEFIGHTER)
	{
		SetAction(o, SelectActionByWeapon(c,
			PLAYER_RAGE_FENRIR_WALK_TWO_SWORD,
			PLAYER_RAGE_FENRIR_WALK_ONE_RIGHT,
			PLAYER_RAGE_FENRIR_WALK_ONE_LEFT,
			PLAYER_RAGE_FENRIR_WALK));
		return;
	}
#endif //PBG_ADD_NEWCHAR_MONK_ANI

	SetAction(o, SelectActionByWeaponWithBow(c,
		PLAYER_FENRIR_WALK_WEAPON_TWO,
		PLAYER_FENRIR_WALK_WEAPON_RIGHT,
		PLAYER_FENRIR_WALK_WEAPON_LEFT,
		PLAYER_FENRIR_WALK));
}

bool IsAliceRideAction_UniDino(unsigned short byAction)
{
	return byAction == PLAYER_SKILL_CHAIN_LIGHTNING_UNI
		|| byAction == PLAYER_SKILL_CHAIN_LIGHTNING_DINO
		|| byAction == PLAYER_SKILL_LIGHTNING_ORB_UNI
		|| byAction == PLAYER_SKILL_LIGHTNING_ORB_DINO
		|| byAction == PLAYER_SKILL_DRAIN_LIFE_UNI
		|| byAction == PLAYER_SKILL_DRAIN_LIFE_DINO;
}

bool IsAliceRideAction_Fenrir(unsigned short byAction)
{
	return byAction == PLAYER_SKILL_CHAIN_LIGHTNING_FENRIR
		|| byAction == PLAYER_SKILL_LIGHTNING_ORB_FENRIR
		|| byAction == PLAYER_SKILL_DRAIN_LIFE_FENRIR;
}

// ---------------------------------------------------------------------------
// Ação e utilitários
// ---------------------------------------------------------------------------

// Troca a ação do objeto. Se bBlending for false, a transição começa do frame 0.
void SetAction(OBJECT* o, int Action, bool bBlending)
{
	BMD* b = gmClientModels->GetModel(o->Type);
	if (!b || Action < 0 || Action >= b->NumActions)
		return;

	if (o->CurrentAction != Action)
	{
		o->PriorAction = o->CurrentAction;
		o->PriorAnimationFrame = o->AnimationFrame;
		o->CurrentAction = Action;
		o->AnimationFrame = 0;

		if (!bBlending)
			o->PriorAnimationFrame = 0;
	}
}

// Verifica se o personagem está dentro de Range (distância no plano XY, sem raiz).
bool TestDistance(CHARACTER* c, vec3_t TargetPosition, float Range)
{
	vec3_t Diff;
	VectorSubtract(c->Object.Position, TargetPosition, Diff);

	const float DistSq = Diff[0] * Diff[0] + Diff[1] * Diff[1];
	return DistSq <= Range * Range;
}

// Ajusta a rotação da cabeça para olhar o alvo, se estiver na frente.
void LookAtTarget(OBJECT* o, CHARACTER* tc)
{
	if (tc == NULL)
		return;

	const OBJECT* to = &tc->Object;
	const float Angle = CreateAngle(o->Position[0], o->Position[1], to->Position[0], to->Position[1]);

	if (FarAngle(o->Angle[2], Angle) < 90.f)
	{
		o->HeadTargetAngle[0] = o->Angle[2] - Angle;
		o->HeadTargetAngle[1] = (to->Position[2] - (o->Position[2] + 50.f)) * 0.2f;

		for (int i = 0; i < 2; i++)
		{
			if (o->HeadTargetAngle[i] < 0.f)
				o->HeadTargetAngle[i] += 360.f;
		}
	}
	else
	{
		o->HeadTargetAngle[0] = 0.f;
		o->HeadTargetAngle[1] = 0.f;
	}
}

// Anima a cabeça do monstro conforme o estado atual.
void MoveHead(CHARACTER* c)
{
	OBJECT* o = &c->Object;

	if (o->CurrentAction == MONSTER01_DIE)
		return;

	if (o->CurrentAction == MONSTER01_STOP1)
	{
		if (rand() % 32 == 0)
		{
			o->HeadTargetAngle[0] = (float)(rand() % 128 - 64);
			o->HeadTargetAngle[1] = (float)(rand() % 48 - 16);

			for (int i = 0; i < 2; i++)
			{
				if (o->HeadTargetAngle[i] < 0.f)
					o->HeadTargetAngle[i] += 360.f;
			}
		}
	}
	else if (o->CurrentAction == MONSTER01_WALK && c->TargetCharacter != -1)
	{
		LookAtTarget(o, gmCharacters->GetCharacter(c->TargetCharacter));
	}
	else
	{
		o->HeadTargetAngle[0] = 0.f;
		o->HeadTargetAngle[1] = 0.f;
	}
}

// Função de dano desativada. O código original tinha 'return;' no início,
// então nada era executado. Se não for mais usada, remova esta função e a declaração no header.
void Damage(vec3_t /*soPosition*/, CHARACTER* /*tc*/, float /*AttackRange*/, int /*AttackPoint*/, bool /*Hit*/)
{
}

// ---------------------------------------------------------------------------
// Caminho (path following)
// ---------------------------------------------------------------------------

// Pesos de Catmull-Rom para os 4 pontos, por subetapa (0..3).
static const float kSplineWeights[4][4] =
{
	{ -0.0703125f, 0.8671875f,  0.2265625f, -0.0234375f },
	{ -0.0625f,    0.5625f,     0.5625f,    -0.0625f    },
	{ -0.0234375f, 0.2265625f,  0.8671875f, -0.0703125f },
	{  0.f,        0.f,         1.f,         0.f         }
};

// Move o personagem ao longo do caminho. Retorna true ao chegar ao destino.
bool MovePath(CHARACTER* c, bool bTurn)
{
	PATH_t* p = &c->Path;
	if (p->CurrentPath >= p->PathNum)
		return false;

	bool bSuccess = false;

	if (p->CurrentPathFloat == 0)
	{
		const int next = min(p->CurrentPath + 1, p->PathNum - 1);
		c->PositionX = p->PathX[next];
		c->PositionY = p->PathY[next];
	}

	// Pontos de controle (4) ao redor do segmento atual
	float x[4], y[4];
	for (int i = 0; i < 4; i++)
	{
		int idx = p->CurrentPath + i - 1;
		idx = max(0, min(idx, p->PathNum - 1));

		x[i] = ((float)p->PathX[idx] + 0.5f) * TERRAIN_SCALE;
		y[i] = ((float)p->PathY[idx] + 0.5f) * TERRAIN_SCALE;
	}

	// Ponto interpolado na subetapa atual
	const float* w = kSplineWeights[p->CurrentPathFloat];
	float cx = 0.f, cy = 0.f;
	for (int i = 0; i < 4; i++)
	{
		cx += x[i] * w[i];
		cy += y[i] * w[i];
	}

	OBJECT* o = &c->Object;
	const float dx = o->Position[0] - cx;
	const float dy = o->Position[1] - cy;

	if (sqrtf(dx * dx + dy * dy) <= 20.f)
	{
		p->CurrentPathFloat++;

		if (p->CurrentPathFloat > 3)
		{
			p->CurrentPathFloat = 0;
			p->CurrentPath++;

			if (p->CurrentPath >= p->PathNum - 1)
			{
				p->CurrentPath = p->PathNum - 1;
				c->PositionX = p->PathX[p->CurrentPath];
				c->PositionY = p->PathY[p->CurrentPath];
				o->Position[0] = cx;
				o->Position[1] = cy;
				bSuccess = true;
			}
		}
	}

	if (!bSuccess && bTurn)
	{
		const float Angle = CreateAngle(o->Position[0], o->Position[1], cx, cy);
		const float TargetAngle = FarAngle(o->Angle[2], Angle);

		if (TargetAngle >= 45.f)
			o->Angle[2] = Angle;
		else
			o->Angle[2] = TurnAngle2(o->Angle[2], Angle, TargetAngle * 0.5f);
	}

	return bSuccess;
}

#ifdef SAVE_PATH_TIME
void WriteDebugInfoStr(const char* lpszFileName, const char* lpszToWrite)
{
	HANDLE hFile = CreateFile(lpszFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL,
		OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return;

	SetFilePointer(hFile, 0, NULL, FILE_END);

	DWORD dwWritten = 0;
	WriteFile(hFile, lpszToWrite, (DWORD)strlen(lpszToWrite), &dwWritten, NULL);
	CloseHandle(hFile);
}

void DebugUtil_Write(const char* lpszFileName, const char* lpszFormat, ...)
{
	char szBuffer[1024];

	va_list va;
	va_start(va, lpszFormat);
	_vsnprintf(szBuffer, sizeof(szBuffer) - 1, lpszFormat, va);
	va_end(va);
	szBuffer[sizeof(szBuffer) - 1] = '\0';

	WriteDebugInfoStr(lpszFileName, szBuffer);
}
#endif

PATH* path = new PATH;

void InitPath()
{
	path->SetMapDimensions(256, 256, TerrainWall);
}

// Busca caminho de (sx,sy) até (tx,ty) e copia o resultado para 'a'.
bool PathFinding2(int sx, int sy, int tx, int ty, PATH_t* a, float fDistance, int iDefaultWall)
{
	bool bIgnoreAltar = false;
	int  Wall = iDefaultWall;

	if (M34CryWolf1st::Get_State_Only_Elf() && M34CryWolf1st::IsCyrWolf1st())
	{
		CHARACTER* pTarget = gmCharacters->GetCharacter(TargetNpc);
		if (pTarget &&
			pTarget->Object.Type >= MODEL_CRYWOLF_ALTAR1 &&
			pTarget->Object.Type <= MODEL_CRYWOLF_ALTAR5)
		{
			bIgnoreAltar = true;
		}
	}

	bool Success = path->FindPath(sx, sy, tx, ty, true, Wall, bIgnoreAltar, fDistance);

	if (!Success)
	{
		const bool bStartSafe = (TerrainWall[TERRAIN_INDEX_REPEAT(sx, sy)] & TW_SAFEZONE) == TW_SAFEZONE;
		const bool bTargetSafe = (TerrainWall[TERRAIN_INDEX_REPEAT(tx, ty)] & TW_SAFEZONE) == TW_SAFEZONE;
		const bool bTargetHasChar = (TerrainWall[TERRAIN_INDEX_REPEAT(tx, ty)] & TW_CHARACTER) == TW_CHARACTER;

		if ((bStartSafe || bTargetSafe) && !bTargetHasChar)
			Wall = TW_NOMOVE;

		Success = path->FindPath(sx, sy, tx, ty, false, Wall, bIgnoreAltar, fDistance);
	}

	if (Success)
	{
		const int PathNum = path->GetPath();
		if (PathNum > 1)
		{
			a->PathNum = PathNum;

			unsigned char* x = path->GetPathX();
			unsigned char* y = path->GetPathY();

			for (int i = 0; i < a->PathNum; i++)
			{
				a->PathX[i] = x[i];
				a->PathY[i] = y[i];
			}

			a->CurrentPath = 0;
			a->CurrentPathFloat = 0;
			return true;
		}
	}
	return false;
}

// ---------------------------------------------------------------------------
// FPS e tempo
// ---------------------------------------------------------------------------

// Atualiza FPS, DeltaT e WorldTime. Chamada uma vez por frame.
void CalcFPS()
{
	static bool  bInit = false;
	static DWORD start = 0;
	static DWORD start2 = 0;
	static DWORD last = 0;
	static int   frame = 0;
	static int   frame2 = 0;

	if (!bInit)
	{
		frame = 0;
		start = timeGetTime();
		last = start;
		bInit = true;
	}

	frame++;
	frame2++;

	const DWORD current = timeGetTime();
	WorldTime = (float)current;

	const int    difTime = (int)(current - last);
	const double dif = (double)(current - start) / CLOCKS_PER_SEC;
	const double rv = (dif != 0.0) ? (double)frame / dif : -1.0;

	if (dif > 2.0 && frame > 10)
	{
		start = start2;
		frame = frame2;
		start2 = timeGetTime();
		frame2 = 0;
	}

	DeltaT = (float)difTime / CLOCKS_PER_SEC;

	// DeltaT nunca deve ser zero (pode ser usado como divisor).
	if (difTime == 0)
		DeltaT = 0.1f / CLOCKS_PER_SEC;

	FPS = (float)rv;
	last = current;

	if (SceneFlag == MAIN_SCENE)
		gSkillManager.CalcSkillDelay(difTime);
}