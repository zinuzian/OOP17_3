////////////////////////////////////////////////////////////////////////////////
//
// File: virtualLego.cpp
//
// Original Author: 박창현 Chang-hyeon Park, 
// Modified by Bong-Soo Sohn and Dong-Jun Kim
// 
// Originally programmed for Virtual LEGO. 
// Modified later to program for Virtual Billiard.
//        
////////////////////////////////////////////////////////////////////////////////

#include "d3dUtility.h"
#include <vector>
#include <ctime>
#include <cstdlib>
#include <cstdio>
#include <cassert>

IDirect3DDevice9* Device = NULL;

// window size
const int Width = 1600;
const int Height = 900;

// There are four balls
// initialize the position (coordinate) of each ball (ball0 ~ ball3)
const float spherePos[4][2] = { { -2.7f, 0 }, { +2.4f, 0 }, { 3.3f, 0 }, { -2.7f, -1.0f } }; // 초기 공 위치 설정, 순서대로  처음 빨간공, 두번째 빨간공, 노란공, 흰공
// initialize the color of each ball (ball0 ~ ball3)
const D3DXCOLOR sphereColor[4] = { d3d::RED, d3d::RED, d3d::YELLOW, d3d::WHITE }; // 공 색 지정


// -----------------------------------------------------------------------------
// Transform matrices
// -----------------------------------------------------------------------------
D3DXMATRIX g_mWorld;
D3DXMATRIX g_mView;
D3DXMATRIX g_mProj;

#define M_RADIUS 0.21   // ball radius
#define PI 3.14159265
#define M_HEIGHT 0.01
#define DECREASE_RATE 0.9982

// -----------------------------------------------------------------------------
// CSphere class definition
// -----------------------------------------------------------------------------

class CSphere {
private:
	float               center_x, center_y, center_z;
	float                   m_radius;
	float               m_velocity_x;
	float               m_velocity_z;

public:
	CSphere(void)
	{
		D3DXMatrixIdentity(&m_mLocal);
		ZeroMemory(&m_mtrl, sizeof(m_mtrl));
		m_radius = 0;
		m_velocity_x = 0;
		m_velocity_z = 0;
		m_pSphereMesh = NULL;
	}
	~CSphere(void) {}

public:
	bool create(IDirect3DDevice9* pDevice, D3DXCOLOR color = d3d::WHITE)
	{
		if (NULL == pDevice)
			return false;

		m_mtrl.Ambient = color;
		m_mtrl.Diffuse = color;
		m_mtrl.Specular = color;
		m_mtrl.Emissive = d3d::BLACK;
		m_mtrl.Power = 5.0f;

		if (FAILED(D3DXCreateSphere(pDevice, getRadius(), 50, 50, &m_pSphereMesh, NULL)))
			return false;
		return true;
	}

	void destroy(void)
	{
		if (m_pSphereMesh != NULL) {
			m_pSphereMesh->Release();
			m_pSphereMesh = NULL;
		}
	}

	void draw(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld)
	{
		if (NULL == pDevice)
			return;
		pDevice->SetTransform(D3DTS_WORLD, &mWorld);
		pDevice->MultiplyTransform(D3DTS_WORLD, &m_mLocal);
		pDevice->SetMaterial(&m_mtrl);
		m_pSphereMesh->DrawSubset(0);
	}

	bool hasIntersected(CSphere& ball)
	{
		// Insert your code here.
		float x1 = this->center_x, z1 = this->center_z;
		float x2 = ball.center_x, z2 = ball.center_z;
		float dist = sqrt(pow((x1 - x2), 2) + pow((z1 - z2), 2));
		if (dist <= (this->getRadius() + ball.getRadius()))
		{
			if (this->center_x >= ball.center_x && this->center_z >= ball.center_z) {
				this->setCenter(this->center_x + M_RADIUS / 100, this->center_y, this->center_z + M_RADIUS / 100);
				ball.setCenter(ball.center_x - M_RADIUS / 100, ball.center_y, ball.center_z - M_RADIUS / 100);
			}
			if (this->center_x >= ball.center_x && this->center_z <= ball.center_z) {
				this->setCenter(this->center_x + M_RADIUS / 100, this->center_y, this->center_z - M_RADIUS / 100);
				ball.setCenter(ball.center_x - M_RADIUS / 100, ball.center_y, ball.center_z + M_RADIUS / 100);
			}
			if (this->center_x <= ball.center_x && this->center_z >= ball.center_z) {
				this->setCenter(this->center_x - M_RADIUS / 100, this->center_y, this->center_z + M_RADIUS / 100);
				ball.setCenter(ball.center_x + M_RADIUS / 100, ball.center_y, ball.center_z - M_RADIUS / 100);
			}
			if (this->center_x <= ball.center_x && this->center_z <= ball.center_z) {
				this->setCenter(this->center_x - M_RADIUS / 100, this->center_y, this->center_z - M_RADIUS / 100);
				ball.setCenter(ball.center_x + M_RADIUS / 100, ball.center_y, ball.center_z + M_RADIUS / 100);
			}
			return true;
		}
		else
			return false;
	}

	void hitBy(CSphere& ball)
	{
		double checkX, checkZ;
		float tempVelocity_X, tempVelocity_Z;
		if (hasIntersected(ball)){
			D3DXVECTOR2 *colVec = new D3DXVECTOR2((float)(ball.center_x - this->center_x), (float)(ball.center_z - this->center_z)),
				*negColVec = new D3DXVECTOR2(-(float)(ball.center_x - this->center_x), -(float)(ball.center_z - this->center_z)),
				*myVec = new D3DXVECTOR2((float)(this->getVelocity_X()), (float)(this->getVelocity_Z())),
				*ballVec = new D3DXVECTOR2((float)(ball.getVelocity_X()), (float)(ball.getVelocity_Z()));


			float size_col, size_my, size_ball;
			size_col = sqrt(pow(colVec->x, 2) + pow(colVec->y, 2));
			size_my = sqrt(pow(myVec->x, 2) + pow(myVec->y, 2));
			size_ball = sqrt(pow(ballVec->x, 2) + pow(ballVec->y, 2));

			D3DXVECTOR2 *d1, *d2, *n1, *n2;   //colVec 좌표 상 벡터
			d1 = new D3DXVECTOR2((*colVec) / size_col); //d1 벡터의 단위벡터
			(*d1) *= (D3DXVec2Dot(colVec, myVec) / size_col);  // d1벡터의 크기
			d2 = new D3DXVECTOR2((*negColVec) / size_col);
			(*d2) *= (D3DXVec2Dot(negColVec, ballVec) / size_col);
			n1 = new D3DXVECTOR2(*myVec - *d1);         //d1 + n1 = myVec
			n2 = new D3DXVECTOR2(*ballVec - *d2);      //d2 + n2 = ballVec

			D3DXVECTOR2* myNewVec, *ballNewVec;
			myNewVec = new D3DXVECTOR2(*d2 + *n1);
			ballNewVec = new D3DXVECTOR2(*d1 + *n2);



			this->setPower(myNewVec->x, myNewVec->y);
			ball.setPower(ballNewVec->x, ballNewVec->y);
		}
	}

	void ballUpdate(float timeDiff)
	{
		const float TIME_SCALE = 3.3;
		D3DXVECTOR3 cord = this->getCenter();
		double vx = abs(this->getVelocity_X());
		double vz = abs(this->getVelocity_Z());

		if (vx > 0.01 || vz > 0.01)
		{
			float tX = cord.x + TIME_SCALE*timeDiff*m_velocity_x;
			float tZ = cord.z + TIME_SCALE*timeDiff*m_velocity_z;


			if (tX >= (4.5 - M_RADIUS))
				tX = 4.5 - M_RADIUS;
			else if (tX <= (-4.5 + M_RADIUS))
				tX = -4.5 + M_RADIUS;
			else if (tZ <= (-3 + M_RADIUS))
				tZ = -3 + M_RADIUS;
			else if (tZ >= (3 - M_RADIUS))
				tZ = 3 - M_RADIUS;

			this->setCenter(tX, cord.y, tZ);
		}
		else { this->setPower(0, 0); }
		//this->setPower(this->getVelocity_X() * DECREASE_RATE, this->getVelocity_Z() * DECREASE_RATE);
		double rate = 1 - (1 - DECREASE_RATE)*timeDiff * 400;
		if (rate < 0)
			rate = 0;
		this->setPower(getVelocity_X() * rate, getVelocity_Z() * rate);
	}

	double getVelocity_X() { return this->m_velocity_x; }
	double getVelocity_Z() { return this->m_velocity_z; }


	void setPower(double vx, double vz)
	{
		this->m_velocity_x = vx;
		this->m_velocity_z = vz;
	}

	void setCenter(float x, float y, float z)
	{
		D3DXMATRIX m;
		center_x = x;   center_y = y;   center_z = z;
		D3DXMatrixTranslation(&m, x, y, z);
		setLocalTransform(m);
	}

	float getRadius(void)  const { return (float)(M_RADIUS); }
	const D3DXMATRIX& getLocalTransform(void) const { return m_mLocal; }
	void setLocalTransform(const D3DXMATRIX& mLocal) { m_mLocal = mLocal; }
	D3DXVECTOR3 getCenter(void) const
	{
		D3DXVECTOR3 org(center_x, center_y, center_z);
		return org;
	}

private:
	D3DXMATRIX              m_mLocal;
	D3DMATERIAL9            m_mtrl;
	ID3DXMesh*              m_pSphereMesh;

};



// -----------------------------------------------------------------------------
// CWall class definition
// -----------------------------------------------------------------------------

class CWall {

private:

	float               m_x;
	float               m_z;
	float                   m_width;
	float                   m_depth;
	float               m_height;

public:
	CWall(void)
	{
		D3DXMatrixIdentity(&m_mLocal);
		ZeroMemory(&m_mtrl, sizeof(m_mtrl));
		m_width = 0;
		m_depth = 0;
		m_pBoundMesh = NULL;
	}
	~CWall(void) {}
public:
	bool create(IDirect3DDevice9* pDevice, float ix, float iz, float iwidth, float iheight, float idepth, D3DXCOLOR color = d3d::WHITE)
	{
		if (NULL == pDevice)
			return false;

		m_mtrl.Ambient = color;
		m_mtrl.Diffuse = color;
		m_mtrl.Specular = color;
		m_mtrl.Emissive = d3d::BLACK;
		m_mtrl.Power = 5.0f;

		m_width = iwidth;
		m_depth = idepth;

		if (FAILED(D3DXCreateBox(pDevice, iwidth, iheight, idepth, &m_pBoundMesh, NULL)))
			return false;
		return true;
	}
	void destroy(void)
	{
		if (m_pBoundMesh != NULL) {
			m_pBoundMesh->Release();
			m_pBoundMesh = NULL;
		}
	}
	void draw(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld)
	{
		if (NULL == pDevice)
			return;
		pDevice->SetTransform(D3DTS_WORLD, &mWorld);
		pDevice->MultiplyTransform(D3DTS_WORLD, &m_mLocal);
		pDevice->SetMaterial(&m_mtrl);
		m_pBoundMesh->DrawSubset(0);
	}

	bool hasIntersected(CSphere& ball)
	{
		for (int i = 0; i < 4; i++){
			if (abs(ball.getCenter().x) >= 4.29f || abs(ball.getCenter().z) >= 2.79f)
				return true;
		}
		return false;
	}

	void hitBy(CSphere& ball)
	{
		D3DXVECTOR3   hitCenter = ball.getCenter();
		if (this->hasIntersected(ball) == true){
			if (ball.getCenter().x >= 4.29f){
				ball.setCenter(4.29f, ball.getCenter().y, ball.getCenter().z);
				ball.setPower(-abs(ball.getVelocity_X()), ball.getVelocity_Z());
			}
			else if (ball.getCenter().x <= -4.29f){
				ball.setCenter(-4.29f, ball.getCenter().y, ball.getCenter().z);
				ball.setPower(abs(ball.getVelocity_X()), ball.getVelocity_Z());
			}
			else if (ball.getCenter().z >= 2.79f){
				ball.setCenter(ball.getCenter().x, ball.getCenter().y, 2.79f);
				ball.setPower(ball.getVelocity_X(), -abs(ball.getVelocity_Z()));
			}
			else if (ball.getCenter().z <= -2.79f){
				ball.setCenter(ball.getCenter().x, ball.getCenter().y, -2.79f);
				ball.setPower(ball.getVelocity_X(), abs(ball.getVelocity_Z()));
			}
		}
	}

	void setPosition(float x, float y, float z)
	{
		D3DXMATRIX m;
		this->m_x = x;
		this->m_z = z;

		D3DXMatrixTranslation(&m, x, y, z);
		setLocalTransform(m);
	}

	float getHeight(void) const { return M_HEIGHT; }



private:
	void setLocalTransform(const D3DXMATRIX& mLocal) { m_mLocal = mLocal; }

	D3DXMATRIX              m_mLocal;
	D3DMATERIAL9            m_mtrl;
	ID3DXMesh*              m_pBoundMesh;
};

// -----------------------------------------------------------------------------
// CLight class definition
// -----------------------------------------------------------------------------

class CLight {
public:
	CLight(void)
	{
		static DWORD i = 0;
		m_index = i++;
		D3DXMatrixIdentity(&m_mLocal);
		::ZeroMemory(&m_lit, sizeof(m_lit));
		m_pMesh = NULL;
		m_bound._center = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
		m_bound._radius = 0.0f;
	}
	~CLight(void) {}
public:
	bool create(IDirect3DDevice9* pDevice, const D3DLIGHT9& lit, float radius = 0.1f)
	{
		if (NULL == pDevice)
			return false;
		if (FAILED(D3DXCreateSphere(pDevice, radius, 10, 10, &m_pMesh, NULL)))
			return false;

		m_bound._center = lit.Position;
		m_bound._radius = radius;

		m_lit.Type = lit.Type;
		m_lit.Diffuse = lit.Diffuse;
		m_lit.Specular = lit.Specular;
		m_lit.Ambient = lit.Ambient;
		m_lit.Position = lit.Position;
		m_lit.Direction = lit.Direction;
		m_lit.Range = lit.Range;
		m_lit.Falloff = lit.Falloff;
		m_lit.Attenuation0 = lit.Attenuation0;
		m_lit.Attenuation1 = lit.Attenuation1;
		m_lit.Attenuation2 = lit.Attenuation2;
		m_lit.Theta = lit.Theta;
		m_lit.Phi = lit.Phi;
		return true;
	}
	void destroy(void)
	{
		if (m_pMesh != NULL) {
			m_pMesh->Release();
			m_pMesh = NULL;
		}
	}
	bool setLight(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld)
	{
		if (NULL == pDevice)
			return false;

		D3DXVECTOR3 pos(m_bound._center);
		D3DXVec3TransformCoord(&pos, &pos, &m_mLocal);
		D3DXVec3TransformCoord(&pos, &pos, &mWorld);
		m_lit.Position = pos;

		pDevice->SetLight(m_index, &m_lit);
		pDevice->LightEnable(m_index, TRUE);
		return true;
	}

	void draw(IDirect3DDevice9* pDevice)
	{
		if (NULL == pDevice)
			return;
		D3DXMATRIX m;
		D3DXMatrixTranslation(&m, m_lit.Position.x, m_lit.Position.y, m_lit.Position.z);
		pDevice->SetTransform(D3DTS_WORLD, &m);
		pDevice->SetMaterial(&d3d::WHITE_MTRL);
		m_pMesh->DrawSubset(0);
	}

	D3DXVECTOR3 getPosition(void) const { return D3DXVECTOR3(m_lit.Position); }

private:
	DWORD               m_index;
	D3DXMATRIX          m_mLocal;
	D3DLIGHT9           m_lit;
	ID3DXMesh*          m_pMesh;
	d3d::BoundingSphere m_bound;
};


// -----------------------------------------------------------------------------
// Global variables
// -----------------------------------------------------------------------------
CWall   g_legoPlane;
CWall   g_legowall[4];
CSphere   g_sphere[4];
CSphere   g_target_blueball;
CLight   g_light;

double g_camera_pos[3] = { 0.0, 5.0, -8.0 };

// -----------------------------------------------------------------------------
// Functions
// -----------------------------------------------------------------------------


void destroyAllLegoBlock(void)
{
}

// initialization
bool Setup()
{
	int i;

	D3DXMatrixIdentity(&g_mWorld);
	D3DXMatrixIdentity(&g_mView);
	D3DXMatrixIdentity(&g_mProj);

	// create plane and set the position
	if (false == g_legoPlane.create(Device, -1, -1, 9, 0.03f, 6, d3d::GREEN)) return false;
	g_legoPlane.setPosition(0.0f, -0.0006f / 5, 0.0f);

	// create walls and set the position. note that there are four walls
	if (false == g_legowall[0].create(Device, -1, -1, 9, 0.3f, 0.12f, d3d::DARKRED)) return false;
	g_legowall[0].setPosition(0.0f, 0.12f, 3.06f);
	if (false == g_legowall[1].create(Device, -1, -1, 9, 0.3f, 0.12f, d3d::DARKRED)) return false;
	g_legowall[1].setPosition(0.0f, 0.12f, -3.06f);
	if (false == g_legowall[2].create(Device, -1, -1, 0.12f, 0.3f, 6.24f, d3d::DARKRED)) return false;
	g_legowall[2].setPosition(4.56f, 0.12f, 0.0f);
	if (false == g_legowall[3].create(Device, -1, -1, 0.12f, 0.3f, 6.24f, d3d::DARKRED)) return false;
	g_legowall[3].setPosition(-4.56f, 0.12f, 0.0f);

	// create four balls and set the position
	for (i = 0; i<4; i++) {
		if (false == g_sphere[i].create(Device, sphereColor[i])) return false;
		g_sphere[i].setCenter(spherePos[i][0], (float)M_RADIUS, spherePos[i][1]);
		g_sphere[i].setPower(0, 0);
	}

	// create blue ball for set direction
	if (false == g_target_blueball.create(Device, d3d::BLUE)) return false;
	g_target_blueball.setCenter(.0f, (float)M_RADIUS, .0f);

	// light setting 
	D3DLIGHT9 lit;
	::ZeroMemory(&lit, sizeof(lit));
	lit.Type = D3DLIGHT_POINT;
	lit.Diffuse = d3d::WHITE;
	lit.Specular = d3d::WHITE * 0.9f;
	lit.Ambient = d3d::WHITE * 0.9f;
	lit.Position = D3DXVECTOR3(0.0f, 3.0f, 0.0f);
	lit.Range = 100.0f;
	lit.Attenuation0 = 0.0f;
	lit.Attenuation1 = 0.9f;
	lit.Attenuation2 = 0.0f;
	if (false == g_light.create(Device, lit))
		return false;

	// Position and aim the camera.
	D3DXVECTOR3 pos(0.0f, 5.0f, -8.0f);
	D3DXVECTOR3 target(0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 up(0.0f, 2.0f, 0.0f);
	D3DXMatrixLookAtLH(&g_mView, &pos, &target, &up);
	Device->SetTransform(D3DTS_VIEW, &g_mView);

	// Set the projection matrix.
	D3DXMatrixPerspectiveFovLH(&g_mProj, D3DX_PI / 4,
		(float)Width / (float)Height, 1.0f, 100.0f);
	Device->SetTransform(D3DTS_PROJECTION, &g_mProj);

	// Set render states.
	Device->SetRenderState(D3DRS_LIGHTING, TRUE);
	Device->SetRenderState(D3DRS_SPECULARENABLE, TRUE);
	Device->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);

	g_light.setLight(Device, g_mWorld);
	return true;
}

void Cleanup(void)
{
	g_legoPlane.destroy();
	for (int i = 0; i < 4; i++) {
		g_legowall[i].destroy();
	}
	destroyAllLegoBlock();
	g_light.destroy();
}


// timeDelta represents the time between the current image frame and the last image frame.
// the distance of moving balls should be "velocity * timeDelta"

bool isBtnPressed = false;
bool isStop = true;
bool whiteTurn = true;
int w_score = 0, y_score = 0;
bool tabPressed = false;

bool Display(float timeDelta)
{
	int i = 0;
	int j = 0;
	static bool status[4][4] = { false };
	

	if (Device)
	{
		Device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x00afafaf, 1.0f, 0);
		Device->BeginScene();

		// update the position of each ball. during update, check whether each ball hit by walls.
		for (i = 0; i < 4; i++) {
			g_sphere[i].ballUpdate(timeDelta);
			for (j = 0; j < 4; j++){ g_legowall[i].hitBy(g_sphere[j]); }
		}

		// check whether any two balls hit together and update the direction of balls
		// 0 1 2 3 빨 빨 노 흰
		for (i = 0; i < 4; i++){
			for (j = 0; j < 4; j++) {		
				if (i >= j) { continue; }
				if (g_sphere[i].hasIntersected((g_sphere[j]))){
					status[i][j] = true;
					status[j][i] = true;
				}
				g_sphere[i].hitBy(g_sphere[j]);
			}
		}

		///////////////show scoreboard
		if (tabPressed){

			// system("pause");


			tabPressed = false;
		}
		///////////////show scoreboard


		//all ball stop
		isStop = true;
		for (int k = 0; k < 4; k++){
			if (g_sphere[k].getVelocity_X() != 0 || g_sphere[k].getVelocity_Z() != 0){
				isStop = false;
			}
		}
		


		int scoreDelta = 0;
		if (isStop){
			if (status[2][3]){
				scoreDelta = -10;
			}
			else if (status[0][3] && status[1][3]){
				scoreDelta = 10;
			}

			if (whiteTurn){
				w_score += scoreDelta;
			}
			else{
				y_score += scoreDelta;
			}


			for (int a = 0; a < 4; a++){
				for (int b = 0; b < 4; b++){
					status[a][b] = false;
				}
			}

			if (isBtnPressed && scoreDelta <= 0){
				CSphere temp;
				temp = g_sphere[3];
				g_sphere[3] = g_sphere[2];
				g_sphere[2] = temp;
				whiteTurn = !whiteTurn;
			}
			isBtnPressed = false;
			///////////////show whose turn now




			///////////////show whose turn now
		}
		

		// draw plane, walls, and spheres
		g_legoPlane.draw(Device, g_mWorld);
		for (i = 0; i<4; i++)    {
			g_legowall[i].draw(Device, g_mWorld);
			g_sphere[i].draw(Device, g_mWorld);
		}
		g_target_blueball.draw(Device, g_mWorld);
		g_light.draw(Device);

		Device->EndScene();
		Device->Present(0, 0, 0, 0);
		Device->SetTexture(0, NULL);
	}
	return true;
}


LRESULT CALLBACK d3d::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static bool wire = false;
	static bool isReset = true;
	static int old_x = 0;
	static int old_y = 0;
	static enum { WORLD_MOVE, LIGHT_MOVE, BLOCK_MOVE } move = WORLD_MOVE;

	switch (msg) {
	case WM_DESTROY:
	{
					   ::PostQuitMessage(0);
					   break;
	}
	case WM_KEYDOWN:
	{
					   
					   switch (wParam) {
					   case 82:					//regame (key R)
						   if (!whiteTurn){
							   CSphere temp;
							   temp = g_sphere[3];
							   g_sphere[3] = g_sphere[2];
							   g_sphere[2] = temp;
						   }
						   isBtnPressed = false;
						   isStop = true;
						   whiteTurn = true;
						   w_score = 0, y_score = 0;

						   for (int i = 0; i < 4; i++){
							   g_sphere[i].setCenter(spherePos[i][0], g_sphere[i].getCenter().y, spherePos[i][1]);
							   g_sphere[i].setPower(0, 0);
						   }

						   break;
					   case 9:					//show score while pressing (tab key)
						   tabPressed = true;

						   break;
					   case VK_ESCAPE:
						   ::DestroyWindow(hwnd);
						   break;
					   case VK_RETURN:
						   if (NULL != Device) {
							   wire = !wire;
							   Device->SetRenderState(D3DRS_FILLMODE,
								   (wire ? D3DFILL_WIREFRAME : D3DFILL_SOLID));
						   }
						   break;
					   case VK_SPACE:
						   if (!isStop){
							   break;
						   }
						   D3DXVECTOR3 targetpos = g_target_blueball.getCenter();
						   D3DXVECTOR3   whitepos = g_sphere[3].getCenter();
						   double theta = acos(sqrt(pow(targetpos.x - whitepos.x, 2)) / sqrt(pow(targetpos.x - whitepos.x, 2) + pow(targetpos.z - whitepos.z, 2)));      // 기본 1 사분면
						   if (targetpos.z - whitepos.z <= 0 && targetpos.x - whitepos.x >= 0) { theta = -theta; }   //4 사분면
						   if (targetpos.z - whitepos.z >= 0 && targetpos.x - whitepos.x <= 0) { theta = PI - theta; } //2 사분면
						   if (targetpos.z - whitepos.z <= 0 && targetpos.x - whitepos.x <= 0){ theta = PI + theta; } // 3 사분면
						   double distance = sqrt(pow(targetpos.x - whitepos.x, 2) + pow(targetpos.z - whitepos.z, 2));
						   g_sphere[3].setPower(distance * cos(theta), distance * sin(theta));
						   isBtnPressed = true;

						   break;

					  
					   }
					   break;
					   
	}

	case WM_MOUSEMOVE:
	{
						 int new_x = LOWORD(lParam);
						 int new_y = HIWORD(lParam);
						 float dx;
						 float dy;

						 if (LOWORD(wParam) & MK_LBUTTON) {

							 if (isReset) {
								 isReset = false;
							 }
							 else {
								 D3DXVECTOR3 vDist;
								 D3DXVECTOR3 vTrans;
								 D3DXMATRIX mTrans;
								 D3DXMATRIX mX;
								 D3DXMATRIX mY;

								 switch (move) {
								 case WORLD_MOVE:
									 dx = (old_x - new_x) * 0.01f;
									 dy = (old_y - new_y) * 0.01f;
									 D3DXMatrixRotationY(&mX, dx);
									 D3DXMatrixRotationX(&mY, dy);
									 g_mWorld = g_mWorld * mX * mY;

									 break;
								 }
							 }

							 old_x = new_x;
							 old_y = new_y;

						 }
						 else {
							 isReset = true;

							 if (LOWORD(wParam) & MK_RBUTTON) {
								 dx = (old_x - new_x);// * 0.01f;
								 dy = (old_y - new_y);// * 0.01f;

								 D3DXVECTOR3 coord3d = g_target_blueball.getCenter();
								 g_target_blueball.setCenter(coord3d.x + dx*(-0.007f), coord3d.y, coord3d.z + dy*0.007f);
							 }
							 old_x = new_x;
							 old_y = new_y;

							 move = WORLD_MOVE;
						 }
						 break;
	}
	}

	return ::DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hinstance,
	HINSTANCE prevInstance,
	PSTR cmdLine,
	int showCmd)
{
	srand(static_cast<unsigned int>(time(NULL)));

	if (!d3d::InitD3D(hinstance,
		Width, Height, true, D3DDEVTYPE_HAL, &Device))
	{
		::MessageBox(0, "InitD3D() - FAILED", 0, 0);
		return 0;
	}

	if (!Setup())
	{
		::MessageBox(0, "Setup() - FAILED", 0, 0);
		return 0;
	}

	d3d::EnterMsgLoop(Display);

	Cleanup();

	Device->Release();

	return 0;
}
