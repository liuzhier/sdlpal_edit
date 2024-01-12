/* -*- mode: c; tab-width: 4; c-basic-offset: 3; c-file-style: "linux" -*- */
//
// Copyright (c) 2009, Wei Mingzhi <whistler_wmz@users.sf.net>.
// All rights reserved.
//
// Portions based on PALx Project by palxex.
// Copyright (c) 2006, Pal Lockheart <palxex@gmail.com>.
//
// This file is part of SDLPAL.
//
// SDLPAL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "cscene.h"
#include "cpalbaseio.h"
#include <assert.h>
#include <algorithm>


CScene::CScene()
{
}



VOID CScene::PAL_AddSpriteToDraw(
	LPCBITMAPRLE     lpSpriteFrame,
	int              x,
	int              y,
	int              iLayer
)
/*++
   Purpose:

   Add a sprite to our list of drawing.

   Parameters:

   [IN]  lpSpriteFrame - the bitmap of the sprite frame.

   [IN]  x - the X coordinate on the screen.

   [IN]  y - the Y coordinate on the screen.

   [IN]  iLayer - the layer of the sprite.

   Return value:

   None.

   --*/
{
	assert(g_rgSpriteToDraw.size() < MAX_SPRITE_TO_DRAW);
	SPRITE_TO_DRAW s{0};
	s.lpSpriteFrame = lpSpriteFrame;
	s.pos = PAL_XY(x, y);
	s.iLayer = iLayer;
	g_rgSpriteToDraw.push_back(s);
	//g_nSpriteToDraw++;
}

VOID CScene::PAL_CalcCoverTiles(SPRITE_TO_DRAW* lpSpriteToDraw
)
/*++
   Purpose:

   Calculate all the tiles which may cover the specified sprite. Add the tiles
   into our list as well.

   Parameters:

   [IN]  lpSpriteToDraw - pointer to SPRITE_TO_DRAW struct.

   Return value:

   None.

   --*/
{
	int             x, y, i, l, iTileHeight;
	LPCBITMAPRLE    lpTile;

	const int       sx = PAL_X(gpGlobals->viewport) + PAL_X(lpSpriteToDraw->pos);
	const int       sy = PAL_Y(gpGlobals->viewport) + PAL_Y(lpSpriteToDraw->pos);
	const int       sh = ((sx % 32) ? 1 : 0);

	const int       width = PAL_RLEGetWidth(lpSpriteToDraw->lpSpriteFrame);
	const int       height = PAL_RLEGetHeight(lpSpriteToDraw->lpSpriteFrame);

	int             dx = 0;
	int             dy = 0;
	int             dh = 0;

	//
	// Loop through all the tiles in the area of the sprite.
	//
	for (y = (sy - height - 15) / 16; y <= sy / 16; y++)
	{
		for (x = (sx - width / 2) / 32; x <= (sx + width / 2) / 32; x++)
		{
			for (i = ((x == (sx - width / 2) / 32) ? 0 : 3); i < 5; i++)
			{
				//
				// Scan tiles in the following form (* = to scan):
				//
				// . . . * * * . . .
				//  . . . * * . . . .
				//
				switch (i)
				{
				case 0:
					dx = x;
					dy = y;
					dh = sh;
					break;

				case 1:
					dx = x - 1;
					break;

				case 2:
					dx = (sh ? x : (x - 1));
					dy = (sh ? (y + 1) : y);
					dh = 1 - sh;
					break;

				case 3:
					dx = x + 1;
					dy = y;
					dh = sh;
					break;

				case 4:
					dx = (sh ? (x + 1) : x);
					dy = (sh ? (y + 1) : y);
					dh = 1 - sh;
					break;
				}

				for (l = 0; l < 2; l++)
				{
					lpTile = PAL_MapGetTileBitmap(dx, dy, dh, l, PAL_GetCurrentMap());
					iTileHeight = (signed char)PAL_MapGetTileHeight(dx, dy, dh, l, PAL_GetCurrentMap());

					//
					// Check if this tile may cover the sprites
					//
					if (lpTile != NULL && iTileHeight > 0 && (dy + iTileHeight) * 16 + dh * 8 >= sy)
					{
						//
						// This tile may cover the sprite
						//
						PAL_AddSpriteToDraw(lpTile,
							dx * 32 + dh * 16 - 16 - PAL_X(gpGlobals->viewport),
							dy * 16 + dh * 8 + 7 + l + iTileHeight * 8 - PAL_Y(gpGlobals->viewport),
							iTileHeight * 8 + l);
					}
				}
			}
		}
	}
}

VOID CScene::PAL_SceneDrawSprites(
	CGL_Texture* iText
)
/*++
   Purpose:

   Draw all the sprites to scene.

   Parameters:

   None.

   Return value:

   None.

   --*/
{
	int i, x, y, vy;

	//g_nSpriteToDraw = 0;
	g_rgSpriteToDraw.clear();
	//
	// Put all the sprites to be drawn into our array.
	//

	//
	// Players
	//
	for (i = 0; i <= (short)gpGlobals->wMaxPartyMemberIndex + gpGlobals->nFollower; i++)
	{
		LPCBITMAPRLE lpBitmap =
			PAL_SpriteGetFrame(PAL_GetPlayerSprite((BYTE)i), gpGlobals->rgParty[i].wFrame);

		if (lpBitmap == NULL)
		{
			continue;
		}

		//
		// Add it to our array
		//
		PAL_AddSpriteToDraw(lpBitmap,
			gpGlobals->rgParty[i].x - PAL_RLEGetWidth(lpBitmap) / 2,
			gpGlobals->rgParty[i].y + gpGlobals->wLayer + 10,
			gpGlobals->wLayer + 6);

		//
		// Calculate covering tiles on the map
		//
		PAL_CalcCoverTiles(&g_rgSpriteToDraw[g_rgSpriteToDraw.size() - 1]);
	}

	//
	// Event Objects (Monsters/NPCs/others)
	//
	for (i = gpGlobals->g.rgScene[gpGlobals->wNumScene - 1].wEventObjectIndex;
		i < gpGlobals->g.rgScene[gpGlobals->wNumScene].wEventObjectIndex; i++)
	{
		LPCBITMAPRLE     lpFrame;
		LPCSPRITE        lpSprite;

		LPEVENTOBJECT    lpEvtObj = &(gpGlobals->g.lprgEventObject[i]);

		int              iFrame;

		if (lpEvtObj->sState == kObjStateHidden || lpEvtObj->sVanishTime > 0 ||
			lpEvtObj->sState < 0)
		{
			continue;
		}

		//
		// Get the sprite
		//
		lpSprite = PAL_GetEventObjectSprite((WORD)i + 1);
		if (lpSprite == NULL)
		{
			continue;
		}

		iFrame = lpEvtObj->wCurrentFrameNum;
		if (lpEvtObj->nSpriteFrames == 3)
		{
			//
			// walking character
			//
			if (iFrame == 2)
			{
				iFrame = 0;
			}

			if (iFrame == 3)
			{
				iFrame = 2;
			}
		}

		lpFrame = PAL_SpriteGetFrame(lpSprite,
			lpEvtObj->wDirection * lpEvtObj->nSpriteFrames + iFrame);

		if (lpFrame == NULL)
		{
			continue;
		}

		//
		// Calculate the coordinate and check if outside the screen
		//
		x = (SHORT)lpEvtObj->x - PAL_X(gpGlobals->viewport);
		x -= PAL_RLEGetWidth(lpFrame) / 2;


		if (x >= 320 || x < -(int)PAL_RLEGetWidth(lpFrame))
		{
			//
			// outside the screen; skip it
			//
			continue;
		}

		y = (SHORT)lpEvtObj->y - PAL_Y(gpGlobals->viewport);
		y += lpEvtObj->sLayer * 8 + 9;

		vy = y - PAL_RLEGetHeight(lpFrame) - lpEvtObj->sLayer * 8 + 2;
		if (vy >= 200 || vy < -(int)PAL_RLEGetHeight(lpFrame))
		{
			//
			// outside the screen; skip it
			//
			continue;
		}

		//
		// Add it into the array
		//
		PAL_AddSpriteToDraw(lpFrame, x, y, lpEvtObj->sLayer * 8 + 2);

		//
		// Calculate covering map tiles
		//
		PAL_CalcCoverTiles(&g_rgSpriteToDraw[g_rgSpriteToDraw.size() - 1]);
	}

	//
	// All sprites are now in our array; sort them by their vertical positions.
	//
#if(1)
	sort(g_rgSpriteToDraw.begin(), g_rgSpriteToDraw.end(), 
		[](SPRITE_TO_DRAW& a, SPRITE_TO_DRAW& b)->BOOL {return PAL_Y(a.pos) < PAL_Y(b.pos); });
#else
	for (x = 0; x < g_rgSpriteToDraw.size() - 1; x++)
	{
		SPRITE_TO_DRAW           tmp;
		BOOL                     fSwap = FALSE;

		for (y = 0; y < g_rgSpriteToDraw.size() - 1 - x; y++)
		{
			if (PAL_Y(g_rgSpriteToDraw[y].pos) > PAL_Y(g_rgSpriteToDraw[y + 1].pos))
			{
				fSwap = TRUE;

				tmp = g_rgSpriteToDraw[y];
				g_rgSpriteToDraw[y] = g_rgSpriteToDraw[y + 1];
				g_rgSpriteToDraw[y + 1] = tmp;
			}
		}

		if (!fSwap)
		{
			break;
		}
	}
#endif
	//
	// Draw all the sprites to the screen.
	//
	for (i = 0; i < g_rgSpriteToDraw.size(); i++)
	{
		SPRITE_TO_DRAW* p = &g_rgSpriteToDraw[i];

		x = PAL_X(p->pos);
		y = PAL_Y(p->pos) - PAL_RLEGetHeight(p->lpSpriteFrame) - p->iLayer;

		PAL_RLEBlitToTexture(p->lpSpriteFrame, iText, PAL_XY(x, y), 0, 0);
	}
	if (gConfig->is_SHOW_OBJECT)
		PAL_Drow_Object();
}

VOID CScene::PAL_Drow_Object(VOID)
{
	int i;// , x, y, vy;


	//
	// Event Objects (Monsters/NPCs/others)
	//
	for (i = gpGlobals->g.rgScene[gpGlobals->wNumScene - 1].wEventObjectIndex;
		i < gpGlobals->g.rgScene[gpGlobals->wNumScene].wEventObjectIndex; i++)
	{
		LPEVENTOBJECT    lpEvtObj = &(gpGlobals->g.lprgEventObject[i]);

		if (lpEvtObj->sState == kObjStateHidden ||
			lpEvtObj->sState < 0)
		{
			continue;
		}


		if(1)//测试   对象的座标显示出来
		{
			PAL_RLEBlitToTexture(PAL_SpriteGetFrame(gpSpriteUI, 68), gpTextureReal,
				PAL_XY((SHORT)lpEvtObj->x - PAL_X(gpGlobals->viewport) - 3,
					(SHORT)lpEvtObj->y - PAL_Y(gpGlobals->viewport) - 5));
		}
	}
}



VOID CScene::PAL_MakeScene(
	VOID
)
/*++
   Purpose:

   Draw the scene of the current frame to the screen. Both the map and
   the sprites are handled here.

   Parameters:

   None.

   Return value:

   None.

   --*/
{

	static SDL_Rect         rect = { 0, 0, 320, 200 };
	/////
	PAL_SetPalette(gpGlobals->wNumPalette, gpGlobals->fNightPalette);
	CGL_Texture* mSurf = new CGL_Texture(320, 200, 32);
	//SDL_Surface* mSurf = SDL_CreateRGBSurface(0, 320, 200, 8, 0, 0, 0, 0);
	//SDL_SetSurfacePalette(mSurf, gpPalette);

	//
	// Step 1: Draw the complete map, for both of the layers.
	//
	rect.x = PAL_X(gpGlobals->viewport);
	rect.y = PAL_Y(gpGlobals->viewport);

	PAL_MapBlitToSurface(PAL_GetCurrentMap(), mSurf, &rect, 0);
	PAL_MapBlitToSurface(PAL_GetCurrentMap(), mSurf, &rect, 1);

	//RenderBlendCopy(gpTextureReal, mSurf);
	//
	// Step 2: Apply screen waving effects.
	//
	PAL_ApplyWave(mSurf);

	//
	// Step 3: Draw all the sprites.
	//
	PAL_SceneDrawSprites(mSurf);

	////
	//
	// Check if we need to fade in.
	//
	if (gpGlobals->fNeedToFadeIn)
	{
		//VIDEO_UpdateScreen(gpTextureReal);
		PAL_FadeIn(gpGlobals->wNumPalette, gpGlobals->fNightPalette, 1);
		gpGlobals->fNeedToFadeIn = FALSE;
	}

	//SDL_FreeSurface(mSurf);
	RenderBlendCopy(gpTextureReal, mSurf);
	delete mSurf;
}

BOOL CScene::PAL_CheckObstacle(
	PAL_POS         pos,
	BOOL            fCheckEventObjects,
	WORD            wSelfObject
)
/*++
   Purpose:

   Check if the specified location has obstacle or not.

   Parameters:

   [IN]  pos - the position to check.

   [IN]  fCheckEventObjects - TRUE if check for event objects, FALSE if only
   check for the map.

   [IN]  wSelfObject - the event object which will be skipped.

   Return value:

   TRUE if the location is obstacle, FALSE if not.

   --*/
{
	int x, y, h, xr, yr;

	if (PAL_X(pos) < 0 || PAL_X(pos) >= 2048 || PAL_Y(pos) < 0 || PAL_Y(pos) >= 2048)
	{
		return TRUE;
	}

	//
	// Check if the map tile at the specified position is blocking
	//
	x = PAL_X(pos) / 32;
	y = PAL_Y(pos) / 16;
	h = 0;

	xr = PAL_X(pos) % 32;
	yr = PAL_Y(pos) % 16;

	if (xr + yr * 2 >= 16)
	{
		if (xr + yr * 2 >= 48)
		{
			x++;
			y++;
		}
		else if (32 - xr + yr * 2 < 16)
		{
			x++;
		}
		else if (32 - xr + yr * 2 < 48)
		{
			h = 1;
		}
		else
		{
			y++;
		}
	}

	if (PAL_MapTileIsBlocked(x, y, h, PAL_GetCurrentMap()))
	{
		return TRUE;
	}

	if (fCheckEventObjects)
	{
		//
		// Loop through all event objects in the current scene
		//
		int i;
		for (i = gpGlobals->g.rgScene[gpGlobals->wNumScene - 1].wEventObjectIndex;
			i < gpGlobals->g.rgScene[gpGlobals->wNumScene].wEventObjectIndex; i++)
		{
			LPEVENTOBJECT p = &(gpGlobals->g.lprgEventObject[i]);
			if (i == wSelfObject - 1)
			{
				//
				// Skip myself
				//
				continue;
			}

			//
			// Is this object a blocking one?
			//
			if (p->sState >= kObjStateBlocker)
			{
				//
				// Check for collision
				//
				if (abs(p->x - PAL_X(pos)) + abs(p->y - PAL_Y(pos)) * 2 < 16)
				{
					return TRUE;
				}
			}
		}
	}

	return FALSE;
}

VOID CScene::PAL_UpdatePartyGestures(
	BOOL             fWalking
)
/*++
   Purpose:

   Update the gestures of all the party members.

   Parameters:

   [IN]  fWalking - whether the party is walking or not.

   Return value:

   None.

   --*/
{
	static int       s_iThisStepFrame = 0;
	int              iStepFrameFollower = 0, iStepFrameLeader = 0;
	int              i;

	if (fWalking)
	{
		//
		// Update the gesture for party leader
		//
		s_iThisStepFrame = (s_iThisStepFrame + 1) % 4;
		if (s_iThisStepFrame & 1)
		{
			iStepFrameLeader = (s_iThisStepFrame + 1) / 2;
			iStepFrameFollower = 3 - iStepFrameLeader;
		}
		else
		{
			iStepFrameLeader = 0;
			iStepFrameFollower = 0;
		}

		gpGlobals->rgParty[0].x = PAL_X(gpGlobals->partyoffset);
		gpGlobals->rgParty[0].y = PAL_Y(gpGlobals->partyoffset);

		if (gpGlobals->g.PlayerRoles.rgwWalkFrames[gpGlobals->rgParty[0].wPlayerRole] == 4)
		{
			gpGlobals->rgParty[0].wFrame = gpGlobals->wPartyDirection * 4 + s_iThisStepFrame;
		}
		else
		{
			gpGlobals->rgParty[0].wFrame = gpGlobals->wPartyDirection * 3 + iStepFrameLeader;
		}

		//
		// Update the gestures and positions for other party members
		//
		for (i = 1; i <= (short)gpGlobals->wMaxPartyMemberIndex; i++)
		{
			gpGlobals->rgParty[i].x = gpGlobals->rgTrail[i - 1].x - PAL_X(gpGlobals->viewport);
			gpGlobals->rgParty[i].y = gpGlobals->rgTrail[i - 1].y - PAL_Y(gpGlobals->viewport);

			gpGlobals->rgParty[i].x +=
				((gpGlobals->rgTrail[i - 1].wDirection == kDirWest || gpGlobals->rgTrail[i - 1].wDirection == kDirSouth) ? 16 : -16);
			gpGlobals->rgParty[i].y +=
				((gpGlobals->rgTrail[i - 1].wDirection == kDirWest || gpGlobals->rgTrail[i - 1].wDirection == kDirNorth) ? 8 : -8);
			//
			// Adjust the position if there is obstacle
			//
			if (PAL_CheckObstacle(PAL_XY(gpGlobals->rgParty[i].x + PAL_X(gpGlobals->viewport),
				gpGlobals->rgParty[i].y + PAL_Y(gpGlobals->viewport)), TRUE, 0))
			{
				gpGlobals->rgParty[i].x = gpGlobals->rgTrail[i - 1].x - PAL_X(gpGlobals->viewport);
				gpGlobals->rgParty[i].y = gpGlobals->rgTrail[i - 1].y - PAL_Y(gpGlobals->viewport);
			}
			//
			// Update gesture for this party member
			//
			if (gpGlobals->g.PlayerRoles.rgwWalkFrames[gpGlobals->rgParty[i].wPlayerRole] == 4)
			{
				gpGlobals->rgParty[i].wFrame = gpGlobals->rgTrail[i - 1].wDirection * 4 + s_iThisStepFrame;
			}
			else
			{
				gpGlobals->rgParty[i].wFrame = gpGlobals->rgTrail[i - 1].wDirection * 3 + iStepFrameLeader;
			}
		}

		if (gpGlobals->nFollower > 0)
		{
			//
			// Update the position and gesture for the follower
			//
			gpGlobals->rgParty[gpGlobals->wMaxPartyMemberIndex + 1].x =
				gpGlobals->rgTrail[gpGlobals->wMaxPartyMemberIndex + 1].x - PAL_X(gpGlobals->viewport);
			gpGlobals->rgParty[gpGlobals->wMaxPartyMemberIndex + 1].y =
				gpGlobals->rgTrail[gpGlobals->wMaxPartyMemberIndex + 1].y - PAL_Y(gpGlobals->viewport);
			gpGlobals->rgParty[gpGlobals->wMaxPartyMemberIndex + 1].wFrame =
				gpGlobals->rgTrail[gpGlobals->wMaxPartyMemberIndex + 1].wDirection * 3 + iStepFrameFollower;
		}
	}
	else
	{
		//
		// Player is not moved. Use the "standing" gesture instead of "walking" one.
		//
		i = gpGlobals->g.PlayerRoles.rgwWalkFrames[gpGlobals->rgParty[0].wPlayerRole];
		if (i == 0)
		{
			i = 3;
		}
		gpGlobals->rgParty[0].wFrame = gpGlobals->wPartyDirection * i;

		for (i = 1; i <= (short)gpGlobals->wMaxPartyMemberIndex; i++)
		{
			int f = gpGlobals->g.PlayerRoles.rgwWalkFrames[gpGlobals->rgParty[i].wPlayerRole];
			if (f == 0)
			{
				f = 3;
			}
			gpGlobals->rgParty[i].wFrame = gpGlobals->rgTrail[2].wDirection * f;
		}

		if (gpGlobals->nFollower > 0)
		{
			gpGlobals->rgParty[gpGlobals->wMaxPartyMemberIndex + 1].wFrame =
				gpGlobals->rgTrail[3].wDirection * 3;
		}

		s_iThisStepFrame &= 2;
		s_iThisStepFrame ^= 2;
	}
}

VOID CScene::PAL_UpdateParty(
	VOID
)
/*++
   Purpose:

   Update the location and walking gesture of all the party members.

   Parameters:

   None.

   Return value:

   None.

   --*/
{
	int              xSource, ySource, xTarget, yTarget, xOffset, yOffset, i;

	//
	// Has user pressed one of the arrow keys?
	//
	if (g_InputState.dir != kDirUnknown)
	{
		xOffset = ((g_InputState.dir == kDirWest || g_InputState.dir == kDirSouth) ? -16 : 16);
		yOffset = ((g_InputState.dir == kDirWest || g_InputState.dir == kDirNorth) ? -8 : 8);

		xSource = PAL_X(gpGlobals->viewport) + PAL_X(gpGlobals->partyoffset);
		ySource = PAL_Y(gpGlobals->viewport) + PAL_Y(gpGlobals->partyoffset);

		xTarget = xSource + xOffset;
		yTarget = ySource + yOffset;

		gpGlobals->wPartyDirection = g_InputState.dir;

		//
		// Check for obstacles on the destination location
		//
		if (!PAL_CheckObstacle(PAL_XY(xTarget, yTarget), TRUE, 0))
		{
			//
			// Player will actually be moved. Store trail.
			//
			for (i = 3; i >= 0; i--)
			{
				gpGlobals->rgTrail[i + 1] = gpGlobals->rgTrail[i];
			}

			gpGlobals->rgTrail[0].wDirection = g_InputState.dir;
			gpGlobals->rgTrail[0].x = xSource;
			gpGlobals->rgTrail[0].y = ySource;

			//
			// Move the viewport
			//
			gpGlobals->viewport =
				PAL_XY(PAL_X(gpGlobals->viewport) + xOffset, PAL_Y(gpGlobals->viewport) + yOffset);

			//
			// Update gestures
			//
			PAL_UpdatePartyGestures(TRUE);
			return; // don't go further
		}
	}

	PAL_UpdatePartyGestures(FALSE);

}

VOID CScene::PAL_NPCWalkOneStep(
	WORD          wEventObjectID,
	INT           iSpeed
)
/*++
  Purpose:

  Move and animate the specified event object (NPC).

  Parameters:

  [IN]  wEventObjectID - the event object to move.

  [IN]  iSpeed - speed of the movement.

  Return value:

  None.

  --*/
{
	LPEVENTOBJECT        p;

	//
	// Check for invalid parameters
	//
	if (wEventObjectID == 0 || wEventObjectID > gpGlobals->g.nEventObject)
	{
		return;
	}

	p = &(gpGlobals->g.lprgEventObject[wEventObjectID - 1]);

	//
	// Move the event object by the specified direction
	//
	p->x += ((p->wDirection == kDirWest || p->wDirection == kDirSouth) ? -2 : 2) * iSpeed;
	p->y += ((p->wDirection == kDirWest || p->wDirection == kDirNorth) ? -1 : 1) * iSpeed;

	//
	// Update the gesture
	//
	if (p->nSpriteFrames > 0)
	{
		p->wCurrentFrameNum++;
		p->wCurrentFrameNum %= (p->nSpriteFrames == 3 ? 4 : p->nSpriteFrames);
	}
	else if (p->nSpriteFramesAuto > 0)
	{
		p->wCurrentFrameNum++;
		p->wCurrentFrameNum %= p->nSpriteFramesAuto;
	}
}

BOOL CScene::PAL_NPCWalkTo(
	WORD           wEventObjectID,
	INT            x,
	INT            y,
	INT            h,
	INT            iSpeed
)
/*++
  Purpose:

  Make the specified event object walk to the map position specified by (x, y, h)
  at the speed of iSpeed.

  Parameters:

  [IN]  wEventObjectID - the event object to move.

  [IN]  x - Column number of the tile.

  [IN]  y - Line number in the map.

  [IN]  h - Each line in the map has two lines of tiles, 0 and 1.
  (See map.h for details.)

  [IN]  iSpeed - the speed to move.

  Return value:

  TRUE if the event object has successfully moved to the specified position,
  FALSE if still need more moving.

  --*/
{
	LPEVENTOBJECT    pEvtObj;
	int              xOffset, yOffset;

	pEvtObj = &(gpGlobals->g.lprgEventObject[wEventObjectID - 1]);

	xOffset = (x * 32 + h * 16) - pEvtObj->x;
	yOffset = (y * 16 + h * 8) - pEvtObj->y;

	if (yOffset < 0)
	{
		pEvtObj->wDirection = ((xOffset < 0) ? kDirWest : kDirNorth);
	}
	else
	{
		pEvtObj->wDirection = ((xOffset < 0) ? kDirSouth : kDirEast);
	}

	if (abs(xOffset) < iSpeed * 2 || abs(yOffset) < iSpeed * 2)
	{
		pEvtObj->x = x * 32 + h * 16;
		pEvtObj->y = y * 16 + h * 8;
	}
	else
	{
		PAL_NPCWalkOneStep(wEventObjectID, iSpeed);
	}

	if (pEvtObj->x == x * 32 + h * 16 && pEvtObj->y == y * 16 + h * 8)
	{
		pEvtObj->wCurrentFrameNum = 0;
		return TRUE;
	}

	return FALSE;
}