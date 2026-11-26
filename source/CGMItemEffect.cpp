#include "stdafx.h"
#include "ZzzEffect.h"
#include "supportingfeature.h"
#include "CGMItemEffect.h"

CGMItemEffect::CGMItemEffect()
{
}

CGMItemEffect::~CGMItemEffect()
{
	csItemEffect.clear();
}

CGMItemEffect* CGMItemEffect::Instance()
{
	static CGMItemEffect sInstance;
	return &sInstance;
}

void CGMItemEffect::LoadDataEffect(int sup23, CUSTOM_EFFECT_STACK* info, int Size)
{
	for (size_t i = 0; i < (size_t)Size; i++)
	{
		if (info[i].isUsable() == false)
			return;

		this->push_back(info[i].ItemIndex, sup23, info[i]);
	}
}

void CGMItemEffect::push_back(int ItemIndex, int sup23, CUSTOM_EFFECT_STACK info)
{
	if (sup23 == 0)
		csItemEffect[ItemIndex].cstackeffect[0].push_back(info);
	else
		csItemEffect[ItemIndex].cstackeffect[1].push_back(info);
}

EFFECT_STACK* CGMItemEffect::Find(int ItemIndex)
{
	type_effect::iterator li = this->csItemEffect.find(ItemIndex);

	if (li == this->csItemEffect.end())
	{
		return NULL;
	}
	return &li->second;
}

void CGMItemEffect::RenderLinkEffect(int ItemIndex, BMD* b, OBJECT* o)
{
	if (ItemIndex >= 0 && ItemIndex < MAX_ITEM)
	{
		EFFECT_STACK* csMake = this->Find(ItemIndex);

		if (csMake == NULL)
			return;

		CreateObjectEffect(0, b, o, &csMake->cstackeffect[0]);

		CreateObjectEffect(1, b, o, &csMake->cstackeffect[1]);
	}
}

void CGMItemEffect::CreateObjectEffect(int sup23, BMD* b, OBJECT* o, std::vector<CUSTOM_EFFECT_STACK>* csMake)
{
	if (csMake != NULL && csMake->size() != 0)
	{
		vec3_t Position, Angle, vLight;

		for (size_t i = 0; i < csMake->size(); i++)
		{
			CUSTOM_EFFECT_STACK* f = &(*csMake)[i];

			f->NormalizeColor(vLight);
			if (sup23 == 0)
			{
				b->TransformByObjectBone(Position, o, f->iBoneNumber, NULL);
				CreateSprite(f->Texture, Position, f->Scale, vLight, o, f->Angle, f->SubType);
			}
			else
			{
				Vector(0.0, 0.0, f->Angle, Angle);
				b->TransformByObjectBone(Position, o, f->iBoneNumber, NULL);
				CreateParticleSync(f->Texture, Position, Angle, vLight, f->SubType, f->Scale, o);
			}
		}
	}
}

void CGMItemEffect::RenderPetEffect(int ItemIndex, BMD* b, OBJECT* o)
{
	if (ItemIndex >= 0 && ItemIndex < MAX_ITEM)
	{
		EFFECT_STACK* csMake = this->Find(ItemIndex);

		if (csMake == NULL)
			return;

		CreatePetEffect(0, b, o, &csMake->cstackeffect[0]);

		CreatePetEffect(1, b, o, &csMake->cstackeffect[1]);
	}
}

void CGMItemEffect::CreatePetEffect(int sup23, BMD* b, OBJECT* o, std::vector<CUSTOM_EFFECT_STACK>* csMake)
{
	if (csMake != NULL && csMake->size() != 0)
	{
		vec3_t Position, Angle, vLight;

		for (size_t i = 0; i < csMake->size(); i++)
		{
			CUSTOM_EFFECT_STACK* f = &(*csMake)[i];

			// TransformByObjectBone valida o indice contra NumBones do modelo,
			// usa SEMPRE a matriz do proprio objeto (nunca a global compartilhada)
			// e soma a posicao atual do objeto. O codigo anterior usava
			// TransformPosition() com fallback para a matriz global, fazendo o
			// efeito (brilho do horse / olhos do fenrir) aparecer solto pelo mapa
			// quando outro modelo animava a matriz global ou o slot era reaproveitado.
			if (f->iBoneNumber >= 0 && f->iBoneNumber < MAX_BONES && o != NULL && b != NULL)
			{
				f->NormalizeColor(vLight);

				b->TransformByObjectBone(Position, o, f->iBoneNumber, NULL);

				if (sup23 == 0)
				{
					CreateSprite(f->Texture, Position, f->Scale, vLight, o, f->Angle, f->SubType);
				}
				else
				{
					Vector(0.0, 0.0, f->Angle, Angle);
					CreateParticleSync(f->Texture, Position, Angle, vLight, f->SubType, f->Scale, o);
				}
			}
		}
	}
}



