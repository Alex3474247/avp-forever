#include "3dc.h"

#include "inline.h"
#include "module.h"
#include "gamedef.h"
#include "stratdef.h"
#include "dynblock.h"
#include "bh_types.h"
#include "avpview.h"

#include "kshape.h"
#include "kzsort.h"
#include "frustrum.h"
#include "vision.h"
#include "lighting.h"
#include "weapons.h"
#include "sfx.h"
/* character extents data so you know where the player's eyes are */
#include "extents.h"
#include "avp_userprofile.h"

#define UseLocalAssert TRUE
#include "ourasert.h"

/* KJL 13:59:05 04/19/97 - avpview.c
 *
 *	This is intended to be an AvP-specific streamlined version of view.c. 
 */
																		
extern void AllNewModuleHandler(void);
extern SCREENDESCRIPTORBLOCK ScreenDescriptorBlock;

DISPLAYBLOCK *OnScreenBlockList[maxobjects];
int NumOnScreenBlocks;

extern DISPLAYBLOCK *ActiveBlockList[];
extern int NumActiveBlocks;

extern DPID MultiplayerObservedPlayer;

#if SupportMorphing
MORPHDISPLAY MorphDisplay;
#endif

#if SupportModules
SCENEMODULE **Global_ModulePtr = 0;
MODULE *Global_MotherModule;
char *ModuleCurrVisArray = 0;
char *ModulePrevVisArray = 0;
char *ModuleTempArray = 0;
char *ModuleLocalVisArray = 0;
int ModuleArraySize = 0;
#endif

/* KJL 11:12:10 06/06/97 - orientation */
MATRIXCH LToVMat;
EULER LToVMat_Euler;
MATRIXCH WToLMat = {1,};
VECTORCH LocalView;

/* KJL 11:16:37 06/06/97 - lights */
VECTORCH LocalLightCH;
int NumLightSourcesForObject;
LIGHTBLOCK *LightSourcesForObject[MaxLightsPerObject];
int GlobalAmbience;
int LightScale=ONE_FIXED;
int DrawingAReflection;

int *Global_ShapePoints;
int **Global_ShapeItems;
int *Global_ShapeNormals;
int *Global_ShapeVNormals;
int **Global_ShapeTextures;
VIEWDESCRIPTORBLOCK *Global_VDB_Ptr;
DISPLAYBLOCK *Global_ODB_Ptr;
SHAPEHEADER *Global_ShapeHeaderPtr;
EXTRAITEMDATA *Global_EID_Ptr;
int *Global_EID_IPtr;


extern float CameraZoomScale;
extern int CameraZoomLevel;
extern int AlienBiteAttackInProgress=0;

/* phase for cloaked objects */
int CloakingPhase;
extern int NormalFrameTime;

int LeanScale;
EULER deathTargetOrientation = {0,0,0};

extern void ColourFillBackBuffer(int FillColour);
void CheckIfMirroringIsRequired(void);
static void ModifyHeadOrientation(void);
void FlushD3DZBuffer();
void UpdateAllFMVTextures();
void D3D_DrawBackdrop(void);
int AVPViewVolumePlaneTest(CLIPPLANEBLOCK *cpb, DISPLAYBLOCK *dblockptr, int or);

#if defined(_MSC_VER)
#define stricmp		_stricmp
#endif

void UpdateRunTimeLights(void)
{
	extern int NumActiveBlocks;
	extern DISPLAYBLOCK *ActiveBlockList[];
	int numberOfObjects = NumActiveBlocks;

	while (numberOfObjects--)
	{
		DISPLAYBLOCK *dispPtr = ActiveBlockList[numberOfObjects];

		if( (dispPtr->SpecialFXFlags & SFXFLAG_ONFIRE)
		  ||((dispPtr->ObStrategyBlock)&&(dispPtr->ObStrategyBlock->SBDamageBlock.IsOnFire)) )
			AddLightingEffectToObject(dispPtr,LFX_OBJECTONFIRE);

		UpdateObjectLights(dispPtr);
	}

	HandleLightElementSystem();
}

void LightSourcesInRangeOfObject(DISPLAYBLOCK *dptr)
{

	DISPLAYBLOCK **aptr;
	DISPLAYBLOCK *dptr2;
	LIGHTBLOCK *lptr;
	VECTORCH llocal;
	int i, j;


	aptr = ActiveBlockList;


	NumLightSourcesForObject = 0;


	/*

	Light Sources attached to other objects

	*/

	for(i = NumActiveBlocks; i!=0 && NumLightSourcesForObject < MaxLightsPerObject; i--) 
	{
		dptr2 = *aptr++;

		if (dptr2->ObNumLights) 
		{
			for(j = 0; j < dptr2->ObNumLights && NumLightSourcesForObject < MaxLightsPerObject; j++) 
			{
				lptr = dptr2->ObLights[j];

				if (!lptr->LightBright || !(lptr->RedScale||lptr->GreenScale||lptr->BlueScale))
				{
					 continue;
				}

				if ((CurrentVisionMode == VISION_MODE_IMAGEINTENSIFIER) && (lptr->LightFlags & LFlag_PreLitSource))
					 continue;
//				lptr->LightFlags |= LFlag_NoSpecular;

		   		if(!(dptr->ObFlags3 & ObFlag3_PreLit &&
					lptr->LightFlags & LFlag_PreLitSource))
				{
					{
						VECTORCH vertexToLight;
						int distanceToLight;

						if (DrawingAReflection)
						{
							vertexToLight.vx = (MirroringAxis - lptr->LightWorld.vx) - dptr->ObWorld.vx;
						}
						else
						{
							vertexToLight.vx = lptr->LightWorld.vx - dptr->ObWorld.vx;
						}
						vertexToLight.vy = lptr->LightWorld.vy - dptr->ObWorld.vy;
						vertexToLight.vz = lptr->LightWorld.vz - dptr->ObWorld.vz;

						distanceToLight = Approximate3dMagnitude(&vertexToLight);

						#if 0
						if (CurrentVisionMode == VISION_MODE_IMAGEINTENSIFIER)
							distanceToLight /= 2;
						#endif

						if(distanceToLight < (lptr->LightRange + dptr->ObRadius))
						{

							LightSourcesForObject[NumLightSourcesForObject] = lptr;
							NumLightSourcesForObject++;

							/* Transform the light position to local space */

							llocal = vertexToLight;

							RotateAndCopyVector(&llocal, &lptr->LocalLP, &WToLMat);
						}
					}
				}
			}
		}
	}

	{
		extern LIGHTELEMENT LightElementStorage[];
		extern int NumActiveLightElements;
		int i = NumActiveLightElements;
		LIGHTELEMENT *lightElementPtr = LightElementStorage;
		while(i--)
		{
			LIGHTBLOCK *lptr = &(lightElementPtr->LightBlock);
			VECTORCH vertexToLight;
			int distanceToLight;

			vertexToLight.vx = lptr->LightWorld.vx - dptr->ObWorld.vx;
			vertexToLight.vy = lptr->LightWorld.vy - dptr->ObWorld.vy;
			vertexToLight.vz = lptr->LightWorld.vz - dptr->ObWorld.vz;

			distanceToLight = Approximate3dMagnitude(&vertexToLight);

			#if 0
			if (CurrentVisionMode == VISION_MODE_IMAGEINTENSIFIER)
				distanceToLight /= 2;
			#endif

			if(distanceToLight < (lptr->LightRange + dptr->ObRadius) )
			{

				LightSourcesForObject[NumLightSourcesForObject] = lptr;
				NumLightSourcesForObject++;

				/* Transform the light position to local space */
				llocal = vertexToLight;
				RotateAndCopyVector(&llocal, &lptr->LocalLP, &WToLMat);
			}

			lightElementPtr++;
		}
	}
}

EULER HeadOrientation = {0,0,0};

static void ModifyHeadOrientation(void)
{
	extern int NormalFrameTime;
	#define TILT_THRESHOLD 128
	PLAYER_STATUS *playerStatusPtr;
    
	/* get the player status block ... */
	playerStatusPtr = (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
    GLOBALASSERT(playerStatusPtr);
  
    if (!playerStatusPtr->IsAlive && !MultiplayerObservedPlayer)
	{
		int decay = NormalFrameTime>>6;

		HeadOrientation.EulerX &= 4095;
	   	HeadOrientation.EulerX -= decay;
		if(HeadOrientation.EulerX < 3072)
			HeadOrientation.EulerX = 3072;
	}
	else
	{
		int decay = NormalFrameTime>>8;
		if(HeadOrientation.EulerX > 2048)
		{
			if (HeadOrientation.EulerX < 4096 - TILT_THRESHOLD)
				HeadOrientation.EulerX = 4096 - TILT_THRESHOLD;

		   	HeadOrientation.EulerX += decay;
			if(HeadOrientation.EulerX > 4095)
				HeadOrientation.EulerX =0;
		}
		else
		{
			if (HeadOrientation.EulerX > TILT_THRESHOLD)
				HeadOrientation.EulerX = TILT_THRESHOLD;

		   	HeadOrientation.EulerX -= decay;
			if(HeadOrientation.EulerX < 0)
				HeadOrientation.EulerX =0;
		}

		if(HeadOrientation.EulerY > 2048)
		{
			if (HeadOrientation.EulerY < 4096 - TILT_THRESHOLD)
				HeadOrientation.EulerY = 4096 - TILT_THRESHOLD;

		   	HeadOrientation.EulerY += decay;
			if(HeadOrientation.EulerY > 4095)
				HeadOrientation.EulerY =0;
		}
		else
		{
			if (HeadOrientation.EulerY > TILT_THRESHOLD)
				HeadOrientation.EulerY = TILT_THRESHOLD;

		   	HeadOrientation.EulerY -= decay;
			if(HeadOrientation.EulerY < 0)
				HeadOrientation.EulerY =0;
		}
		
		if(HeadOrientation.EulerZ > 2048)
		{
			if (HeadOrientation.EulerZ < 4096 - TILT_THRESHOLD)
				HeadOrientation.EulerZ = 4096 - TILT_THRESHOLD;

		   	HeadOrientation.EulerZ += decay;
			if(HeadOrientation.EulerZ > 4095)
				HeadOrientation.EulerZ =0;
		}
		else
		{
			if (HeadOrientation.EulerZ > TILT_THRESHOLD)
				HeadOrientation.EulerZ = TILT_THRESHOLD;

		   	HeadOrientation.EulerZ -= decay;
			if(HeadOrientation.EulerZ < 0)
				HeadOrientation.EulerZ =0;
		}
	}
}

void InteriorType_Body()
{
	DISPLAYBLOCK *subjectPtr = Player;
	extern int NormalFrameTime;

	static int verticalSpeed = 0;
	static int zAxisTilt=0;
	STRATEGYBLOCK *sbPtr;
	DYNAMICSBLOCK *dynPtr;
	
	sbPtr = subjectPtr->ObStrategyBlock;
	LOCALASSERT(sbPtr);
	dynPtr = sbPtr->DynPtr;	
	LOCALASSERT(dynPtr);
    
	ModifyHeadOrientation();
	{
		/* eye offset */
		VECTORCH ioff;
		COLLISION_EXTENTS *extentsPtr = 0;
		PLAYER_STATUS *playerStatusPtr = (PLAYER_STATUS *)(sbPtr->SBdataptr);

		switch(AvP.PlayerType)
		{
			case I_Marine:
				extentsPtr = &CollisionExtents[CE_MARINE];
				break;
				
			case I_Alien:
				extentsPtr = &CollisionExtents[CE_ALIEN];
				break;
			
			case I_Predator:
				extentsPtr = &CollisionExtents[CE_PREDATOR];
				break;
		}
		
		/* set player state */
		if (playerStatusPtr->ShapeState == PMph_Standing)
		{
			ioff.vy = extentsPtr->StandingTop;
		}
		else
		{
			ioff.vy = extentsPtr->CrouchingTop;
		}

		if (LANDOFTHEGIANTS_CHEATMODE)
		{
			ioff.vy/=4; //sets player eye level to the floor level
		}
		if (!playerStatusPtr->IsAlive && !MultiplayerObservedPlayer)
		{
			extern int deathFadeLevel;
			
			ioff.vy = MUL_FIXED(deathFadeLevel*4-3*ONE_FIXED,ioff.vy);

			if (ioff.vy>-100)
			{
				ioff.vy = -100;
			}
		}
				
		ioff.vx = 0;
		ioff.vz = 0;//-extentsPtr->CollisionRadius*2;
		ioff.vy += verticalSpeed/16+200;

		RotateVector(&ioff, &subjectPtr->ObMat);
		AddVector(&ioff, &Global_VDB_Ptr->VDB_World);
	}
	{
		EULER orientation;
		MATRIXCH matrix;

		orientation = HeadOrientation;

		orientation.EulerZ += (zAxisTilt>>8);
		orientation.EulerZ &= 4095;
		
		if (NAUSEA_CHEATMODE)
		{
			orientation.EulerZ = (orientation.EulerZ+GetSin((CloakingPhase/2)&4095)/256)&4095;
			orientation.EulerX = (orientation.EulerX+GetSin((CloakingPhase/2+500)&4095)/512)&4095;
			orientation.EulerY = (orientation.EulerY+GetSin((CloakingPhase/3+800)&4095)/512)&4095;
		}
		// The next test drops the matrix multiply if the orientation is close to zero
		// There is an inaccuracy problem with the Z angle at this point
		if (orientation.EulerX != 0 || orientation.EulerY != 0 || 
					(orientation.EulerZ > 1 && orientation.EulerZ <	4095))
		{
			CreateEulerMatrix(&orientation, &matrix);
			MatrixMultiply(&Global_VDB_Ptr->VDB_Mat, &matrix, &Global_VDB_Ptr->VDB_Mat);
	 	}
	}

	{
		VECTORCH relativeVelocity;
		
		/* get subject's total velocity */
		{
			MATRIXCH worldToLocalMatrix;

			/* make world to local matrix */
			worldToLocalMatrix = subjectPtr->ObMat;
			TransposeMatrixCH(&worldToLocalMatrix);													   

			relativeVelocity.vx = dynPtr->Position.vx - dynPtr->PrevPosition.vx;		
			relativeVelocity.vy = dynPtr->Position.vy - dynPtr->PrevPosition.vy;
			relativeVelocity.vz = dynPtr->Position.vz - dynPtr->PrevPosition.vz;
			/* rotate into object space */

			RotateVector(&relativeVelocity,&worldToLocalMatrix);
		}
		{
			int targetingSpeed = 10*NormalFrameTime;
	
			/* KJL 14:08:50 09/20/96 - the targeting is FRI, but care has to be taken
			   at very low frame rates to ensure that you can't overshoot */
			if (targetingSpeed > 65536)	targetingSpeed=65536;
					
			zAxisTilt += MUL_FIXED
				(
					DIV_FIXED
					(
						MUL_FIXED(relativeVelocity.vx,LeanScale),
						NormalFrameTime
					)-zAxisTilt,
					targetingSpeed
				);

			{
				static int previousVerticalSpeed = 0;
				int difference;

				if (relativeVelocity.vy >= 0)
				{ 
					difference = DIV_FIXED
					(
						previousVerticalSpeed - relativeVelocity.vy,
						NormalFrameTime
					);
				}
				else difference = 0;

				if (verticalSpeed < difference) verticalSpeed = difference;
				
			 	if(verticalSpeed > 150*16) verticalSpeed = 150*16;
				
				verticalSpeed -= NormalFrameTime>>2;
				if (verticalSpeed < 0) verticalSpeed = 0;				
				
				previousVerticalSpeed = relativeVelocity.vy;
			}
	 	}
	}
}

void UpdateCamera(void)
{
//	char buf[300];
	PLAYER_STATUS *playerStatusPtr = (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
	int cos = GetCos(playerStatusPtr->ViewPanX); // the looking up/down value that used to be in displayblock
	int sin = GetSin(playerStatusPtr->ViewPanX);
	MATRIXCH mat;
	DISPLAYBLOCK *dptr_s = Player;

	// update the two globals
	Global_VDB_Ptr->VDB_World = dptr_s->ObWorld; // world space location
	Global_VDB_Ptr->VDB_Mat = dptr_s->ObMat;	 // local -> world orientation matrix

	// world position
/*
	sprintf(buf, "player world location - x: %d y: %d z: %d\n", Global_VDB_Ptr->VDB_World.vx, Global_VDB_Ptr->VDB_World.vy, Global_VDB_Ptr->VDB_World.vz);
	OutputDebugString(buf);

	sprintf(buf, 
	"\t %d \t %d \t %d\n"
	"\t %d \t %d \t %d\n"
	"\t %d \t %d \t %d\n",
	Global_VDB_Ptr->VDB_Mat.mat11, Global_VDB_Ptr->VDB_Mat.mat12, Global_VDB_Ptr->VDB_Mat.mat13,
	Global_VDB_Ptr->VDB_Mat.mat21, Global_VDB_Ptr->VDB_Mat.mat22, Global_VDB_Ptr->VDB_Mat.mat23,
	Global_VDB_Ptr->VDB_Mat.mat31, Global_VDB_Ptr->VDB_Mat.mat32, Global_VDB_Ptr->VDB_Mat.mat33);
//	OutputDebugString(buf);
*/
	mat.mat11 = ONE_FIXED;
	mat.mat12 = 0;
	mat.mat13 = 0;
	mat.mat21 = 0;
	mat.mat22 = cos;
	mat.mat23 = -sin;
	mat.mat31 = 0;
	mat.mat32 = sin;
	mat.mat33 = cos;
 	MatrixMultiply(&Global_VDB_Ptr->VDB_Mat, &mat, &Global_VDB_Ptr->VDB_Mat);

	InteriorType_Body();
}

void AVPGetInViewVolumeList(VIEWDESCRIPTORBLOCK *VDB_Ptr)
{
	DISPLAYBLOCK **activeblocksptr;
	int t;
	#if (SupportModules && SupportMultiCamModules)
	int MVis;
	#endif

	/* Initialisation */
	NumOnScreenBlocks = 0;

	/* Scan the Active Blocks List */
	activeblocksptr = &ActiveBlockList[0];

	for (t = NumActiveBlocks; t!=0; t--)
	{
		DISPLAYBLOCK *dptr = *activeblocksptr++;

		if (dptr==Player) 
			continue;

		MVis = TRUE;
		if (dptr->ObMyModule)
		{
			MODULE *mptr = dptr->ObMyModule;
			if (ModuleCurrVisArray[mptr->m_index] != 2)
			{
				MVis = FALSE;
			}
			else
			{
				extern int NumberOfLandscapePolygons;
				SHAPEHEADER *shapePtr = GetShapeData(dptr->ObShape);
				NumberOfLandscapePolygons += shapePtr->numitems;
			}
		}
		if (!(dptr->ObFlags&ObFlag_NotVis) && MVis) 
		{
			MakeVector(&dptr->ObWorld, &VDB_Ptr->VDB_World, &dptr->ObView);
			RotateVector(&dptr->ObView, &VDB_Ptr->VDB_Mat);

			/* Screen Test */
			#if MIRRORING_ON
			if (MirroringActive || dptr->HModelControlBlock || dptr->SfxPtr)
			{
				OnScreenBlockList[NumOnScreenBlocks++] = dptr;
			}
			else if (ObjectWithinFrustrum(dptr))
			{
				OnScreenBlockList[NumOnScreenBlocks++] = dptr;
			}
			#else
			if(dptr->SfxPtr || dptr->HModelControlBlock || ObjectWithinFrustrum(dptr))
			{
				OnScreenBlockList[NumOnScreenBlocks++] = dptr;
			}
			else
			{
				if(dptr->HModelControlBlock)
				{
					DoHModelTimer(dptr->HModelControlBlock);
				}
			}
			#endif
		}
	}
}

void ReflectObject(DISPLAYBLOCK *dPtr)
{
	dPtr->ObWorld.vx = MirroringAxis - dPtr->ObWorld.vx;
	dPtr->ObMat.mat11 = -dPtr->ObMat.mat11;
	dPtr->ObMat.mat21 = -dPtr->ObMat.mat21;
	dPtr->ObMat.mat31 = -dPtr->ObMat.mat31;
}

void AvpShowViews(void)
{

	FlushD3DZBuffer();

	UpdateAllFMVTextures();	

	/* Update attached object positions and orientations etc. */
	UpdateCamera();

	/* Initialise the global VMA */
//	GlobalAmbience=655;
//	textprint("Global Ambience: %d\n",GlobalAmbience);
	
	/* Prepare the View Descriptor Block for use in ShowView() */
	PrepareVDBForShowView(Global_VDB_Ptr);
	PlatformSpecificShowViewEntry(Global_VDB_Ptr, &ScreenDescriptorBlock);
	TranslationSetup();

	D3D_DrawBackdrop();

	/* Now we know where the camera is, update the modules */

	#if SupportModules
	AllNewModuleHandler();
//	ModuleHandler(Global_VDB_Ptr);
	#endif

	#if MIRRORING_ON
	CheckIfMirroringIsRequired();
	#endif

	/* Do lights */
	UpdateRunTimeLights();

	if (AvP.PlayerType==I_Alien)
	{
		MakeLightElement(&Player->ObWorld,LIGHTELEMENT_ALIEN_TEETH);
		MakeLightElement(&Player->ObWorld,LIGHTELEMENT_ALIEN_TEETH2);
	}

	/* Find out which objects are in the View Volume */
	AVPGetInViewVolumeList(Global_VDB_Ptr);

	if (AlienBiteAttackInProgress)
	{
		CameraZoomScale += (float)NormalFrameTime/65536.0f;
		if (CameraZoomScale > 1.0f)
		{
			AlienBiteAttackInProgress = 0;
			CameraZoomScale = 1.0f;
		}
	}

	/* update players weapon */
	UpdateWeaponStateMachine();
	/* lights associated with the player may have changed */
	UpdateObjectLights(Player);

	if (NumOnScreenBlocks)
	{
	 	/* KJL 12:13:26 02/05/97 - divert rendering for AvP */
		KRenderItems(Global_VDB_Ptr);
	}

	PlatformSpecificShowViewExit(Global_VDB_Ptr, &ScreenDescriptorBlock);

	/* KJL 10:25:44 7/23/97 - this offset is used to push back the normal game gfx,
	so that the HUD can be drawn over the top without sinking into walls, etc. */
	HeadUpDisplayZOffset = 0;
}


void InitCameraValues(void)
{
	extern VIEWDESCRIPTORBLOCK *ActiveVDBList[];
	Global_VDB_Ptr = ActiveVDBList[0];

	HeadOrientation.EulerX = 0;
	HeadOrientation.EulerY = 0;
	HeadOrientation.EulerZ = 0;

	CameraZoomScale = 1.0f;
	CameraZoomLevel = 0;
}



/*

 Prepare the View Descriptor Block for use in ShowView() and others.

 If there is a display block attached to the view, update the view location
 and orientation.

*/

#include <assert.h>

void PrepareVDBForShowView(VIEWDESCRIPTORBLOCK *VDB_Ptr)
{
	EULER e;

	/* Get the View Object Matrix, transposed */
 	TransposeMatrixCH(&VDB_Ptr->VDB_Mat);

	/* Get the Matrix Euler Angles */
	MatrixToEuler(&VDB_Ptr->VDB_Mat, &VDB_Ptr->VDB_MatrixEuler);

	/* Get the Matrix Euler Angles */
//	MatrixToEuler(&VDB_Ptr->VDB_Mat, &e);

	// bjd - commented out above function call, and do below instead?
	e = VDB_Ptr->VDB_MatrixEuler;

	assert(e.EulerX == VDB_Ptr->VDB_MatrixEuler.EulerX);
	assert(e.EulerY == VDB_Ptr->VDB_MatrixEuler.EulerY);
	assert(e.EulerZ == VDB_Ptr->VDB_MatrixEuler.EulerZ);

	/* Create the "sprite" matrix" */
/* // bjd - only referenced in commented out code
	e.EulerX = 0;
	e.EulerY = 0;
	e.EulerZ = (-e.EulerZ) & wrap360; // wrap360 = 4095

	CreateEulerMatrix(&e, &VDB_Ptr->VDB_SpriteMat);
*/
}

   
/*

 This function updates the position and orientation of the lights attached
 to an object.

 It must be called after the object has completed its movements in a frame,
 prior to the call to the renderer.

*/

void UpdateObjectLights(DISPLAYBLOCK *dptr)
{

	int i;
	LIGHTBLOCK *lptr;
	LIGHTBLOCK **larrayptr = &dptr->ObLights[0];


	for(i = dptr->ObNumLights; i!=0; i--)
	{
		/* Get a light */
		lptr = *larrayptr++;

		/* Calculate the light's location */
		if(!(lptr->LightFlags & LFlag_AbsPos))
		{
			CopyVector(&dptr->ObWorld, &lptr->LightWorld);
     	}
		LOCALASSERT(lptr->LightRange!=0);
		lptr->BrightnessOverRange = DIV_FIXED(MUL_FIXED(lptr->LightBright,LightScale),lptr->LightRange);
	}
}

/****************************************************************************/

/*

 Find out which light sources are in range of the object.

*/




/*

 Initialise the Renderer

*/

void InitialiseRenderer(void)
{
	InitialiseObjectBlocks();
	InitialiseStrategyBlocks();

	InitialiseTxAnimBlocks();

	InitialiseLightBlocks();
	InitialiseVDBs();

	/* KJL 14:46:42 09/09/98 */
	InitialiseLightIntensityStamps();
}





/*

 General View Volume Test for Objects and Sub-Object Trees

 This function returns returns "TRUE" / "True" for an if()

*/

int AVPViewVolumeTest(VIEWDESCRIPTORBLOCK *VDB_Ptr, DISPLAYBLOCK *dblockptr)
{
	int or = dblockptr->ObRadius;

	/* Perform the view volume plane tests */

	if(
	AVPViewVolumePlaneTest(&VDB_Ptr->VDB_ClipZPlane, dblockptr, or) &&
	AVPViewVolumePlaneTest(&VDB_Ptr->VDB_ClipLeftPlane, dblockptr, or) &&
	AVPViewVolumePlaneTest(&VDB_Ptr->VDB_ClipRightPlane, dblockptr, or) &&
	AVPViewVolumePlaneTest(&VDB_Ptr->VDB_ClipUpPlane, dblockptr, or) &&
	AVPViewVolumePlaneTest(&VDB_Ptr->VDB_ClipDownPlane, dblockptr, or))
		return TRUE;

	else
		return FALSE;

}
/*

 View Volume Plane Test

 Make the ODB VSL relative to the VDB Clip Plane POP and dot the resultant
 vector with the Clip Plane Normal.

*/

int AVPViewVolumePlaneTest(CLIPPLANEBLOCK *cpb, DISPLAYBLOCK *dblockptr, int or)
{
	VECTORCH POPRelObView;

	MakeVector(&dblockptr->ObView, &cpb->CPB_POP, &POPRelObView);

	if(DotProduct(&POPRelObView, &cpb->CPB_Normal) < or) return TRUE;
	else return FALSE;
}


#if MIRRORING_ON
void CheckIfMirroringIsRequired(void)
{
	extern char LevelName[];
	extern MODULE * playerPherModule;

	MirroringActive = 0;

	if (!stricmp(LevelName,"derelict"))
	{
		if (playerPherModule && playerPherModule->name)
		{
			if((!stricmp(playerPherModule->name,"start"))
			 ||(!stricmp(playerPherModule->name,"start-en01")) )
			{
				MirroringActive = 1;
				MirroringAxis = -5596*2;
			}
		}
	}
}
#endif

#define MinChangeInXSize 8

void MakeViewingWindowSmaller(void)
{
	extern VIEWDESCRIPTORBLOCK *Global_VDB_Ptr;
	int MinChangeInYSize = (ScreenDescriptorBlock.SDB_Height * MinChangeInXSize) / ScreenDescriptorBlock.SDB_Width;
	
	if (Global_VDB_Ptr->VDB_ClipLeft<ScreenDescriptorBlock.SDB_Width/2-16)
	{
		Global_VDB_Ptr->VDB_ClipLeft +=MinChangeInXSize;
		Global_VDB_Ptr->VDB_ClipRight -=MinChangeInXSize;
		Global_VDB_Ptr->VDB_ClipUp +=MinChangeInYSize;
		Global_VDB_Ptr->VDB_ClipDown -=MinChangeInYSize;
	}
	if(AvP.PlayerType == I_Alien)
	{
		Global_VDB_Ptr->VDB_ProjX = (Global_VDB_Ptr->VDB_ClipRight - Global_VDB_Ptr->VDB_ClipLeft)/4;
		Global_VDB_Ptr->VDB_ProjY = (Global_VDB_Ptr->VDB_ClipDown - Global_VDB_Ptr->VDB_ClipUp)/4;
	}
	else
	{
		Global_VDB_Ptr->VDB_ProjX = (Global_VDB_Ptr->VDB_ClipRight - Global_VDB_Ptr->VDB_ClipLeft)/2;
		Global_VDB_Ptr->VDB_ProjY = (Global_VDB_Ptr->VDB_ClipDown - Global_VDB_Ptr->VDB_ClipUp)/2;
	}
	//BlankScreen(); 
}

void MakeViewingWindowLarger(void)
{
	extern VIEWDESCRIPTORBLOCK *Global_VDB_Ptr;
	int MinChangeInYSize = (ScreenDescriptorBlock.SDB_Height * MinChangeInXSize) / ScreenDescriptorBlock.SDB_Width;

	if (Global_VDB_Ptr->VDB_ClipLeft>0)
	{
		Global_VDB_Ptr->VDB_ClipLeft -=MinChangeInXSize;
		Global_VDB_Ptr->VDB_ClipRight +=MinChangeInXSize;
		Global_VDB_Ptr->VDB_ClipUp -=MinChangeInYSize;
		Global_VDB_Ptr->VDB_ClipDown +=MinChangeInYSize;
	}
	if (AvP.PlayerType == I_Alien)
	{
		Global_VDB_Ptr->VDB_ProjX = (Global_VDB_Ptr->VDB_ClipRight - Global_VDB_Ptr->VDB_ClipLeft)/4;
		Global_VDB_Ptr->VDB_ProjY = (Global_VDB_Ptr->VDB_ClipDown - Global_VDB_Ptr->VDB_ClipUp)/4;
	}
	else
	{
		Global_VDB_Ptr->VDB_ProjX = (Global_VDB_Ptr->VDB_ClipRight - Global_VDB_Ptr->VDB_ClipLeft)/2;
		Global_VDB_Ptr->VDB_ProjY = (Global_VDB_Ptr->VDB_ClipDown - Global_VDB_Ptr->VDB_ClipUp)/2;
	}
}


extern void AlienBiteAttackHasHappened(void)
{
	extern int AlienTongueOffset;
	extern int AlienTeethOffset;

	AlienBiteAttackInProgress = 1;

	CameraZoomScale = 0.25f;
	AlienTongueOffset = ONE_FIXED;
	AlienTeethOffset = 0;
}

