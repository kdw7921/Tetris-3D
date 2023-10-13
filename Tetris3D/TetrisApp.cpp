//***************************************************************************************
// TetrisApp.cpp by Frank Luna (C) 2015 All Rights Reserved.
//
// Hold down '1' key to view scene in wireframe mode.
//***************************************************************************************

#include "Common/d3dApp.h"
#include "Common/MathHelper.h"
#include "Common/UploadBuffer.h"
#include "Common/GeometryGenerator.h"
#include "FrameResource.h"

#include<time.h>

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

const int gNumFrameResources = 3;

// Lightweight structure stores parameters to draw a shape.  This will
// vary from app-to-app.
struct RenderItem
{
	RenderItem() = default;

    // World matrix of the shape that describes the object's local space
    // relative to the world space, which defines the position, orientation,
    // and scale of the object in the world.
    XMFLOAT4X4 World = MathHelper::Identity4x4();

	XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();

	// Dirty flag indicating the object data has changed and we need to update the constant buffer.
	// Because we have an object cbuffer for each FrameResource, we have to apply the
	// update to each FrameResource.  Thus, when we modify obect data we should set 
	// NumFramesDirty = gNumFrameResources so that each frame resource gets the update.
	int NumFramesDirty = gNumFrameResources;

	// Index into GPU constant buffer corresponding to the ObjectCB for this render item.
	UINT ObjCBIndex = -1;

	Material* Mat = nullptr;
	MeshGeometry* Geo = nullptr;

    // Primitive topology.
    D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    // DrawIndexedInstanced parameters.
    UINT IndexCount = 0;
    UINT StartIndexLocation = 0;
    int BaseVertexLocation = 0;
};

enum class RenderLayer : int
{
	Opaque = 0,
	Count
};

class TetrisApp : public D3DApp
{
public:
	TetrisApp(HINSTANCE hInstance);
	TetrisApp(const TetrisApp& rhs) = delete;
	TetrisApp& operator=(const TetrisApp& rhs) = delete;
    ~TetrisApp();

    virtual bool Initialize()override;

	void GameInitialize();

private:
    virtual void OnResize()override;
    virtual void Update(const GameTimer& gt)override;
    virtual void Draw(const GameTimer& gt)override;

    virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
    virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
    virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

	virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);


    void OnKeyboardInput(const GameTimer& gt);
	void UpdateCamera(const GameTimer& gt);
	void UpdateObjectCBs(const GameTimer& gt);
	void UpdateMaterialCBs(const GameTimer& gt);
	void UpdateMainPassCB(const GameTimer& gt);

    void BuildDescriptorHeaps();
    void BuildConstantBufferViews();
    void BuildRootSignature();
    void BuildShadersAndInputLayout();
    void BuildShapeGeometry();
	void BuildMaterials();
    void BuildPSOs();
    void BuildFrameResources();
    void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);

	void BuildbackgrounGrid();

	void BuildRenderItemsOnMap();
	void BuildRenderItemsOnCBlock();

	void SettingForRenderitems();
	void AddSkullRItem(int x, int y);
	void AddRenderItem(unsigned int type, int x, int y);

	void mapInitialize();
	void blockInitialize(int TYPE);
	bool checkOver();
	void checkLine(int y, int* score);
	void removeLine(int i);
	bool blockRoll(int x, int y);
	bool checkBlock(int x, int y, int cblock[5][5]);
	void blockToMap(int x, int y);
	void BlockArrived();

private:

    std::vector<std::unique_ptr<FrameResource>> mFrameResources;
    FrameResource* mCurrFrameResource = nullptr;
    int mCurrFrameResourceIndex = 0;

    ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
    ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr;

	ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;

	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
	std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
	std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
    std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;

    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

	// List of all the render items.
	std::vector<std::unique_ptr<RenderItem>> mAllRitems;

	// Render items divided by PSO.
	std::vector<RenderItem*> mRitemLayer[(int)RenderLayer::Count];

    PassConstants mMainPassCB;

    UINT mPassCbvOffset = 0;

    bool mIsWireframe = false;

	XMFLOAT3 mEyePos = { 0.0f, 0.0f, 0.0f };
	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();

    float mTheta = 1.5f*XM_PI;
    float mPhi = 0.5f*XM_PI;
    float mRadius = 30.0f;

	float mLightTheta = 1.25f*XM_PI;
	float mLightPhi = XM_PIDIV4;

    POINT mLastMousePos;

	UINT mObjCBIndex;

// Tetris Game

	#define HEIGHT 22
	#define WIDTH 11

	bool rotate, flicker1, flicker2, toonShading;

	bool SPACE, DOWN, UP, RIGHT, LEFT;

	int map[HEIGHT][WIDTH];

	unsigned int x = 1, y = 2;

	const int BLOCK[7][5][5] = {
		{ { 9,9,9,9,9 },
		{ 9,9,9,9,9 },
		{ 9,9,4,4,9 },
		{ 9,9,4,4,9 },
		{ 9,9,9,9,9 } },

		{ { 9,9,9,9,9 },
		{ 9,9,0,9,9 },
		{ 9,9,0,9,9 },
		{ 9,9,0,9,9 },
		{ 9,9,0,9,9 } },

		{ { 9,9,9,9,9 },
		{ 9,9,9,9,9 },
		{ 9,9,5,5,9 },
		{ 9,5,5,9,9 },
		{ 9,9,9,9,9 } },

		{ { 9,9,9,9,9 },
		{ 9,9,9,9,9 },
		{ 9,6,6,9,9 },
		{ 9,9,6,6,9 },
		{ 9,9,9,9,9 } },

		{ { 9,9,9,9,9 },
		{ 9,9,1,9,9 },
		{ 9,1,1,1,9 },
		{ 9,9,9,9,9 },
		{ 9,9,9,9,9 } },

		{ { 9,9,9,9,9 },
		{ 9,3,3,9,9 },
		{ 9,9,3,9,9 },
		{ 9,9,3,9,9 },
		{ 9,9,9,9,9 } },

		{ { 9,9,9,9,9 },
		{ 9,9,2,2,9 },
		{ 9,9,2,9,9 },
		{ 9,9,2,9,9 },
		{ 9,9,9,9,9 } }
	};
	int CBlock[5][5];
	int TYPE, NextTYPE;
	int score;

	__int64 flickerStartTime, rotateStartTime,currTimeForBlock, prevTimeForBlock = 0;

	XMFLOAT3 originMat = XMFLOAT3(0.01f, 0.01f, 0.01f);
	XMFLOAT3 water = XMFLOAT3(0.2f, 0.2f, 0.2f);
	XMFLOAT3 glass = XMFLOAT3(0.5f, 0.5f, 0.5f);
	XMFLOAT3 plastic = XMFLOAT3(0.8f, 0.8f, 0.8f);
	XMFLOAT3 gold = XMFLOAT3(1.0f, 0.71f, 0.29f);
	XMFLOAT3 silver = XMFLOAT3(0.95f, 0.93f, 0.88f);
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
    PSTR cmdLine, int showCmd)
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    try
    {
		TetrisApp theApp(hInstance);
        if(!theApp.Initialize())
            return 0;

		
		theApp.GameInitialize();
		return theApp.Run();
    }
    catch(DxException& e)
    {
        MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
        return 0;
    }
}

TetrisApp::TetrisApp(HINSTANCE hInstance)
    : D3DApp(hInstance)
{
}

TetrisApp::~TetrisApp()
{
    if(md3dDevice != nullptr)
        FlushCommandQueue();
}

bool TetrisApp::Initialize()
{
    if(!D3DApp::Initialize())
        return false;

    // Reset the command list to prep for initialization commands.
    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	mapInitialize();

    BuildRootSignature();
    BuildShadersAndInputLayout();
    BuildShapeGeometry();
	BuildMaterials();
    BuildPSOs();

    // Execute the initialization commands.
    ThrowIfFailed(mCommandList->Close());
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    // Wait until initialization is complete.
    FlushCommandQueue();

    return true;
}
void TetrisApp::GameInitialize() {
	score = 0;
	
	rotate = flicker1 = flicker2 = toonShading = false;
	SPACE = DOWN = UP = RIGHT = LEFT = false;

	srand(time(NULL));
	TYPE = rand() % 7;
	NextTYPE = rand() % 7;

	mapInitialize();
	blockInitialize(TYPE);

	x = WIDTH / 2 - 2;
	y = -1;

	BuildRenderItemsOnMap();

	BuildbackgrounGrid();

	BuildRenderItemsOnCBlock();
	SettingForRenderitems();
}

void TetrisApp::SettingForRenderitems() {
	
	FlushCommandQueue();

	for (auto& e : mFrameResources) {
		if(e->CmdListAlloc.Get())	e->CmdListAlloc.ReleaseAndGetAddressOf();
		if(e->ObjectCB)				e->ObjectCB.release();
		if (e->MaterialCB)			e->MaterialCB.release();
		if (e->PassCB)				e->PassCB.release();
	}	
	mFrameResources.clear();
	BuildFrameResources();

	if(mCbvHeap.Get())	mCbvHeap.ReleaseAndGetAddressOf();
	BuildDescriptorHeaps(); 

	BuildConstantBufferViews();
}
 
void TetrisApp::mapInitialize() {
	int x, y;
	for (y = 0; y < HEIGHT; y++) {
		for (x = 0; x < WIDTH; x++) {
			if (y == 1 && x < WIDTH - 1 && x>0) map[y][x] = 8;
			else if (y == HEIGHT - 1 || x == 0 || x == WIDTH - 1) 	map[y][x] = 7;
			else 								map[y][x] = 9;
		}
	}
	return;
}

LRESULT TetrisApp::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
		case WM_KEYDOWN:
			switch (wParam) {
				case VK_RIGHT:
					RIGHT = true;
					break;
				case VK_LEFT:
					LEFT = true;
					break;
				case VK_UP:
					UP = true;
					break;
				case VK_DOWN:
					DOWN = true;
					break;
				case VK_SPACE:
					SPACE = true;
					break;
				case VK_3:
					rotate = rotate ? false : true;
					if (rotate)  QueryPerformanceCounter((LARGE_INTEGER*)&rotateStartTime);
					break;
				case VK_4:
					flicker2 = false;
					flicker1 = flicker1 ? false : true;
					if (flicker1)  QueryPerformanceCounter((LARGE_INTEGER*)&flickerStartTime);
					break;
				case VK_5:
					flicker1 = false;
					flicker2 = flicker2 ? false : true;
					if (flicker2)  QueryPerformanceCounter((LARGE_INTEGER*)&flickerStartTime);
					break;
				case VK_NUMPAD0:
					for (auto& e : mMaterials) {
						Material* mat = e.second.get();
						mat->FresnelR0 = originMat;
						mat->Roughness = 0.125f;
						mat->NumFramesDirty = gNumFrameResources;
					}
					break;
				case VK_NUMPAD1:
					for (auto& e : mMaterials) {
						Material* mat = e.second.get();
						mat->FresnelR0 = water;
						mat->NumFramesDirty = gNumFrameResources;
					}
					break;
				case VK_NUMPAD2:
					for (auto& e : mMaterials) {
						Material* mat = e.second.get();
						mat->FresnelR0 = plastic;
						mat->NumFramesDirty = gNumFrameResources;
					}
					break;
				case VK_NUMPAD3:
					for (auto& e : mMaterials) {
						Material* mat = e.second.get();
						mat->FresnelR0 = gold;
						mat->NumFramesDirty = gNumFrameResources;
					}
					break;
				case VK_NUMPAD4:
					for (auto& e : mMaterials) {
						Material* mat = e.second.get();
						mat->FresnelR0 = silver;
						mat->NumFramesDirty = gNumFrameResources;
					}
					break;
				case VK_NUMPAD7:
					for (auto& e : mMaterials) {
						Material* mat = e.second.get();
						mat->Roughness = MathHelper::Clamp(mat->Roughness - 0.1f, 0.0f, 1.0f);
						mat->NumFramesDirty = gNumFrameResources;
					}
					break;
				case VK_NUMPAD9:
					for (auto& e : mMaterials) {
						Material* mat = e.second.get();
						mat->Roughness = MathHelper::Clamp(mat->Roughness + 0.1f, 0.0f, 1.0f);
						mat->NumFramesDirty = gNumFrameResources;
					}
					break;

				case VK_NUMPAD8:
					toonShading = toonShading ? false : true;
					break;
			}
			return 0;
	}

	return D3DApp::MsgProc(hwnd, msg, wParam, lParam);
}

void TetrisApp::BlockArrived() {
	blockToMap(x, y);

	if (checkOver()) {
		GameInitialize();
	}

	checkLine(y, &score);

	TYPE = NextTYPE;
	NextTYPE = rand() % 7;

	blockInitialize(TYPE);

	x = WIDTH / 2 - 2;
	y = -1;

	BuildRenderItemsOnMap();
	BuildbackgrounGrid();
	BuildRenderItemsOnCBlock();
	SettingForRenderitems();
}

void TetrisApp::blockToMap(int x, int y) {
	int i, j;
	for (i = 0; i < 5; i++) {
		for (j = 0; j < 5; j++) {
			if (CBlock[i][j] != 9) map[y + i][x + j] = CBlock[i][j];
		}
	}
}

void TetrisApp::checkLine(int y, int* score) {
	int i, j;
	bool flag;
	for (i = 0; i < 5; i++) {
		flag = true;
		for (j = 1; j < WIDTH - 1; j++) {
			if (map[y + i][j] > 6) {
				flag = false;
				break;
			}
		}
		if (flag == true) {
			*score += 10;
			removeLine(y + i);
		}
	}
}

void TetrisApp::removeLine(int i) {
	int j;
	for (; i > 0; i--) {
		for (j = 1; j < WIDTH - 1; j++) {
			map[i][j] = map[i - 1][j];
		}
	}
	for (j = 1; j < WIDTH - 1; j++)
		if (map[2][j] == 8) map[2][j] = 9;
	for (j = 1; j < WIDTH - 1; j++)
		if (map[1][j] == 9) map[1][j] = 8;
	for (j = 1; j < WIDTH - 1; j++) map[0][j] = 9;
}

void TetrisApp::blockInitialize(int TYPE) {
	int i, j;
	for (i = 0; i < 5; i++) {
		for (j = 0; j < 5; j++) {
			if (BLOCK[TYPE][i][j] != 9) CBlock[i][j] = BLOCK[TYPE][i][j];
			else CBlock[i][j] = 9;
		}
	}
}

bool TetrisApp::checkBlock(int x, int y, int cblock[5][5]) {
	int i, j;
	for (i = 0; i < 5; i++) {
		for (j = 0; j < 5; j++) {
			if ( (-1 < y + i && y + i < HEIGHT) && (-1 < x + j && x + j < WIDTH) ){
				if (cblock[i][j] < 7 && (map[y + i][x + j] < 8 )) {
					return true;
				}
			}
		}
	}
	return false;
}

bool TetrisApp::blockRoll(int x, int y) {
	int ri, rj;
	int i, j;
	int rblock[5][5];
	for (i = 0, rj = 4; i < 5; rj--, i++) {
		for (j = 0, ri = 0; j < 5; ri++, j++) {
			rblock[i][j] = CBlock[ri][rj];
		}
	}
	if (checkBlock(x, y, rblock) != true) {
		for (i = 0; i < 5; i++) {
			for (j = 0; j < 5; j++) {
				CBlock[i][j] = rblock[i][j];
			}
		}
		return true;
	}
	return false;
}
bool TetrisApp::checkOver() {
	int i;
	for (i = 1; i < WIDTH - 1; i++)
		if (map[1][i] != 8) return true;
	return false;
}

void TetrisApp::OnResize()
{
    D3DApp::OnResize();

    // The window resized, so update the aspect ratio and recompute the projection matrix.
    XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
    XMStoreFloat4x4(&mProj, P);
}

void TetrisApp::Update(const GameTimer& gt)
{

    OnKeyboardInput(gt);
	UpdateCamera(gt);

	//update based on time
	bool flag = false;
	bool arrived = false;
	XMMATRIX world;
	for (auto& e : mAllRitems)
	{
		world = XMLoadFloat4x4(&e->World);

		if (e->ObjCBIndex == mAllRitems.size() - 4) {
			QueryPerformanceCounter((LARGE_INTEGER*)&currTimeForBlock);
			if ((currTimeForBlock - prevTimeForBlock)*gt.mSecondsPerCount >= 1.0f) {
				if (!checkBlock(x, y + 1, CBlock)) {
					y++;
					flag = true;
				}
				else {
					arrived = true;
					break;
				}
				prevTimeForBlock = currTimeForBlock;
			}
		}
		if (e->ObjCBIndex >= mAllRitems.size() - 4) {
			if (flag == true) {
				world *= XMMatrixTranslation(0.0f, -1.0f, 0.0f);
				XMStoreFloat4x4(&e->World, world);
				e->NumFramesDirty = gNumFrameResources;
			}
		}
	}
	if(arrived)
		BlockArrived();

    // Cycle through the circular frame resource array.
    mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
    mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

    // Has the GPU finished processing the commands of the current frame resource?
    // If not, wait until the GPU has completed commands up to this fence point.
    if(mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
    {
        HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
        ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }

	UpdateObjectCBs(gt);
	UpdateMaterialCBs(gt);
	UpdateMainPassCB(gt);
}

void TetrisApp::Draw(const GameTimer& gt)
{
    auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;

    // Reuse the memory associated with command recording.
    // We can only reset when the associated command lists have finished execution on the GPU.
    ThrowIfFailed(cmdListAlloc->Reset());

    // A command list can be reset after it has been added to the command queue via ExecuteCommandList.
    // Reusing the command list reuses memory.
    if(mIsWireframe) {
        ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), 
			mPSOs["opaque_wireframe"].Get())
		);
    }
    else if(toonShading){
        ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), 
			mPSOs["opaque_toonShading"].Get())
		);
    }
	else {
		ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(),
			mPSOs["opaque"].Get())
		);
	}

    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    // Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    // Clear the back buffer and depth buffer.
    mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::Black, 0, nullptr);
    mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    // Specify the buffers we are going to render to.
    mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

    ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvHeap.Get() };
    mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

    int passCbvIndex = mPassCbvOffset + mCurrFrameResourceIndex;
    auto passCbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvHeap->GetGPUDescriptorHandleForHeapStart());
    passCbvHandle.Offset(passCbvIndex, mCbvSrvUavDescriptorSize);
    mCommandList->SetGraphicsRootDescriptorTable(2, passCbvHandle);

    DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Opaque]);

    // Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    // Done recording commands.
    ThrowIfFailed(mCommandList->Close());

    // Add the command list to the queue for execution.
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    // Swap the back and front buffers
    ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

    // Advance the fence value to mark commands up to this fence point.
    mCurrFrameResource->Fence = ++mCurrentFence;
    
    // Add an instruction to the command queue to set a new fence point. 
    // Because we are on the GPU timeline, the new fence point won't be 
    // set until the GPU finishes processing all the commands prior to this Signal().
    mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

void TetrisApp::OnMouseDown(WPARAM btnState, int x, int y)
{
    mLastMousePos.x = x;
    mLastMousePos.y = y;

    SetCapture(mhMainWnd);
}

void TetrisApp::OnMouseUp(WPARAM btnState, int x, int y)
{
    ReleaseCapture();
}

void TetrisApp::OnMouseMove(WPARAM btnState, int x, int y)
{
    if((btnState & MK_LBUTTON) != 0)
    {
        // Make each pixel correspond to a quarter of a degree.
        float dx = XMConvertToRadians(0.25f*static_cast<float>(x - mLastMousePos.x));
        float dy = XMConvertToRadians(0.25f*static_cast<float>(y - mLastMousePos.y));

        // Update angles based on input to orbit camera around box.
        mTheta += dx;
        mPhi += dy;

        // Restrict the angle mPhi.
        mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
    }
    else if((btnState & MK_RBUTTON) != 0)
    {
        // Make each pixel correspond to 0.2 unit in the scene.
        float dx = 0.05f*static_cast<float>(x - mLastMousePos.x);
        float dy = 0.05f*static_cast<float>(y - mLastMousePos.y);

        // Update the camera radius based on input.
        mRadius += dx - dy;

        // Restrict the radius.
        mRadius = MathHelper::Clamp(mRadius, 5.0f, 150.0f);
    }

    mLastMousePos.x = x;
    mLastMousePos.y = y;
}
 
void TetrisApp::OnKeyboardInput(const GameTimer& gt)
{
    if(GetAsyncKeyState('1') & 0x8000)
        mIsWireframe = true;
    else
        mIsWireframe = false;

	if (GetAsyncKeyState('2') & 0x8000) {
		mTheta = 1.5f*XM_PI;
		mPhi = 0.5f*XM_PI;
		mRadius = 30.0f;
	}

	if (RIGHT) {
		if (!checkBlock(x + 1, y, CBlock)) {
			x++;
			for (auto& e : mAllRitems) {
				if (e->ObjCBIndex >= mAllRitems.size() - 4) {
					XMMATRIX world = XMLoadFloat4x4(&e->World) * XMMatrixTranslation(1.0f, 0.0f, 0.0f);
					XMStoreFloat4x4(&e->World, world);
					e->NumFramesDirty = gNumFrameResources;
				}
			}
		}
		RIGHT = false;
	}
	if (LEFT) {
		if (!checkBlock(x - 1, y, CBlock)) {
			x--;
			for (auto& e : mAllRitems) {
				if (e->ObjCBIndex >= mAllRitems.size() - 4) {
					XMMATRIX world = XMLoadFloat4x4(&e->World) * XMMatrixTranslation(-1.0f, 0.0f, 0.0f);
					XMStoreFloat4x4(&e->World, world);
					e->NumFramesDirty = gNumFrameResources;
				}
			}
		}
		LEFT = false;
	}
	if (UP) {
		if (TYPE != 0 && blockRoll(x, y)) {
			for (int i = 0; i < 4; i++) {
				mAllRitems.pop_back();
				mRitemLayer[(int)RenderLayer::Opaque].pop_back();
				mObjCBIndex--;
			}
			for (auto& e : mAllRitems) e->Mat->NumFramesDirty = gNumFrameResources;
			BuildRenderItemsOnCBlock();
			SettingForRenderitems();
		}
		UP = false;
	}
	if (DOWN) {
		if (!checkBlock(x, y + 1, CBlock)) {
			y++;
			for (auto& e : mAllRitems) {
				if (e->ObjCBIndex >= mAllRitems.size() - 4) {
					XMMATRIX world = XMLoadFloat4x4(&e->World) * XMMatrixTranslation(0.0f, -1.0f, 0.0f);
					XMStoreFloat4x4(&e->World, world);
					e->NumFramesDirty = gNumFrameResources;
				}
			}
		}
		DOWN = false;
	}
	if (SPACE) {
		while (!checkBlock(x, y + 1, CBlock)) {
			y++;
		}
		BlockArrived();
		SPACE = false;
	}
}
 
void TetrisApp::UpdateCamera(const GameTimer& gt)
{
	// Convert Spherical to Cartesian coordinates.
	mEyePos.x = mRadius*sinf(mPhi)*cosf(mTheta);
	mEyePos.z = mRadius*sinf(mPhi)*sinf(mTheta);
	mEyePos.y = mRadius*cosf(mPhi);

	// Build the view matrix.
	XMVECTOR pos = XMVectorSet(mEyePos.x, mEyePos.y, mEyePos.z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, view);
}

void TetrisApp::UpdateObjectCBs(const GameTimer& gt)
{
	auto currObjectCB = mCurrFrameResource->ObjectCB.get();

	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
	auto t = (currTime - rotateStartTime) * gt.mSecondsPerCount;
	float rotateSpeed = 0.3;

	for(auto& e : mAllRitems)
	{
		XMMATRIX world = XMLoadFloat4x4(&e->World);
		XMMATRIX texTransform = XMLoadFloat4x4(&e->TexTransform);

		if (rotate) {
			world *= XMMatrixRotationY(t * rotateSpeed * XM_2PI);
		}

		ObjectConstants objConstants;
		XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
		XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));

		currObjectCB->CopyData(e->ObjCBIndex, objConstants);

		if (e->NumFramesDirty > 0)	e->NumFramesDirty--;

		/*
		// Only update the cbuffer data if the constants have changed.  
		// This needs to be tracked per frame resource.
		if(e->NumFramesDirty > 0)
		{
			XMMATRIX world = XMLoadFloat4x4(&e->World);

			ObjectConstants objConstants;
			XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));

			currObjectCB->CopyData(e->ObjCBIndex, objConstants);

			// Next FrameResource need to be updated too.
			e->NumFramesDirty--;
		}
		*/
	}
}

void TetrisApp::UpdateMaterialCBs(const GameTimer& gt)
{
	auto currMaterialCB = mCurrFrameResource->MaterialCB.get();
	for (auto& e : mMaterials)
	{
		// Only update the cbuffer data if the constants have changed.  If the cbuffer
		// data changes, it needs to be updated for each FrameResource.
		Material* mat = e.second.get();
		if (mat->NumFramesDirty > 0)
		{
			XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);

			MaterialConstants matConstants;
			matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
			matConstants.FresnelR0 = mat->FresnelR0;
			matConstants.Roughness = mat->Roughness;
			XMStoreFloat4x4(&matConstants.MatTransform, XMMatrixTranspose(matTransform));

			currMaterialCB->CopyData(mat->MatCBIndex, matConstants);

			// Next FrameResource need to be updated too.
			mat->NumFramesDirty--;
		}
	}
}

void TetrisApp::UpdateMainPassCB(const GameTimer& gt)
{
	XMMATRIX view = XMLoadFloat4x4(&mView);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
	mMainPassCB.EyePosW = mEyePos;
	mMainPassCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
	mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
	mMainPassCB.NearZ = 1.0f;
	mMainPassCB.FarZ = 1000.0f;
	mMainPassCB.TotalTime = gt.TotalTime();
	mMainPassCB.DeltaTime = gt.DeltaTime();
	XMVECTOR AmbientL = { 0.25f, 0.25f, 0.35f, 1.0f };
	XMVECTOR lightStrength[3] = { { 0.6f, 0.6f, 0.6f }, {0.3f, 0.3f, 0.3f}, {0.15f, 0.15f, 0.15f} };

	if (flicker1 || flicker2) {
		__int64 currTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
		auto t = (currTime - flickerStartTime) * gt.mSecondsPerCount;
		float flickerSpeed = 1.0f;

		float trans = (cosf(t * flickerSpeed * XM_PI) + 1) / 2;

		if(flicker2) 
			trans = ( trans > 0.3 ) ? 1.0f : 0.0f;

		AmbientL *= trans;
		lightStrength[0] *= trans;
		lightStrength[1] *= trans;
		lightStrength[2] *= trans;
	}
	XMStoreFloat4(&mMainPassCB.AmbientLight, AmbientL);
	mMainPassCB.Lights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
	XMStoreFloat3(&mMainPassCB.Lights[0].Strength, lightStrength[0]);
	mMainPassCB.Lights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
	XMStoreFloat3(&mMainPassCB.Lights[1].Strength, lightStrength[1]);
	mMainPassCB.Lights[2].Direction = { 0.0f, -0.707f, -0.707f };
	XMStoreFloat3(&mMainPassCB.Lights[2].Strength, lightStrength[2]);

	auto currPassCB = mCurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, mMainPassCB);
}

void TetrisApp::BuildDescriptorHeaps()
{
    UINT objCount = (UINT)mRitemLayer[(int)RenderLayer::Opaque].size();

    // Need a CBV descriptor for each object for each frame resource,
    // +1 for the perPass CBV for each frame resource.
    UINT numDescriptors = (objCount+1) * gNumFrameResources;

    // Save an offset to the start of the pass CBVs.  These are the last 3 descriptors.
    mPassCbvOffset = objCount * gNumFrameResources;

    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
    cbvHeapDesc.NumDescriptors = numDescriptors;
    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    cbvHeapDesc.NodeMask = 0;
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&cbvHeapDesc,
        IID_PPV_ARGS(&mCbvHeap)));
}

void TetrisApp::BuildConstantBufferViews()
{
    UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

    UINT objCount = (UINT)mRitemLayer[(int)RenderLayer::Opaque].size();

    // Need a CBV descriptor for each object for each frame resource.
    for(int frameIndex = 0; frameIndex < gNumFrameResources; ++frameIndex)
    {
        auto objectCB = mFrameResources[frameIndex]->ObjectCB->Resource();
        for(UINT i = 0; i < objCount; ++i)
        {
            D3D12_GPU_VIRTUAL_ADDRESS cbAddress = objectCB->GetGPUVirtualAddress();

            // Offset to the ith object constant buffer in the buffer.
            cbAddress += i*objCBByteSize;

            // Offset to the object cbv in the descriptor heap.
            int heapIndex = frameIndex*objCount + i;
            auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvHeap->GetCPUDescriptorHandleForHeapStart());
            handle.Offset(heapIndex, mCbvSrvUavDescriptorSize);

            D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
            cbvDesc.BufferLocation = cbAddress;
            cbvDesc.SizeInBytes = objCBByteSize;

            md3dDevice->CreateConstantBufferView(&cbvDesc, handle);
        }
    }

    UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));

    // Last three descriptors are the pass CBVs for each frame resource.
    for(int frameIndex = 0; frameIndex < gNumFrameResources; ++frameIndex)
    {
        auto passCB = mFrameResources[frameIndex]->PassCB->Resource();
        D3D12_GPU_VIRTUAL_ADDRESS cbAddress = passCB->GetGPUVirtualAddress();

        // Offset to the pass cbv in the descriptor heap.
        int heapIndex = mPassCbvOffset + frameIndex;
        auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvHeap->GetCPUDescriptorHandleForHeapStart());
        handle.Offset(heapIndex, mCbvSrvUavDescriptorSize);

        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
        cbvDesc.BufferLocation = cbAddress;
        cbvDesc.SizeInBytes = passCBByteSize;
        
        md3dDevice->CreateConstantBufferView(&cbvDesc, handle);
    }
}

void TetrisApp::BuildRootSignature()
{
    CD3DX12_DESCRIPTOR_RANGE cbvTable0;
    cbvTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);

    CD3DX12_DESCRIPTOR_RANGE cbvTable1;
    cbvTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2);

	// Root parameter can be a table, root descriptor or root constants.
	CD3DX12_ROOT_PARAMETER slotRootParameter[3];

	// Create root CBVs.
    slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable0);
	slotRootParameter[1].InitAsConstantBufferView(1);
    slotRootParameter[2].InitAsDescriptorTable(1, &cbvTable1);

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(3, slotRootParameter, 0, nullptr, 
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if(errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}

void TetrisApp::BuildShadersAndInputLayout()
{
	mShaders["standardVS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["opaquePS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "PS", "ps_5_1");
	mShaders["opaqueToonShadingPS"] = d3dUtil::CompileShader(L"Shaders\\toonShading.hlsl", nullptr, "PS", "ps_5_1");
	
    mInputLayout =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
}

void TetrisApp::BuildShapeGeometry()
{
    GeometryGenerator geoGen;
	GeometryGenerator::MeshData box = geoGen.CreateBox(1.0f, 1.0f, 1.0f, 0);
	GeometryGenerator::MeshData backgroundGrid = geoGen.CreateGrid((float)WIDTH-2, (float)HEIGHT-2, HEIGHT-1, WIDTH-1);

	backgroundGrid.Indices32.clear();
	backgroundGrid.Indices32.resize( ((HEIGHT)*(WIDTH) * 2) * 4); // 4 indices per face

	// Iterate over each quad and compute indices.
	{
		int m = HEIGHT-1, n = WIDTH-1;
		unsigned int k = 0;
		for (unsigned int i = 0; i < m-1; ++i)
		{
			for (unsigned int j = 0; j < n-1; ++j)
			{

				backgroundGrid.Indices32[k] = (i + 1)*n + j;
				backgroundGrid.Indices32[k+1] = i * n + j;

				backgroundGrid.Indices32[k + 2] = i * n + j;
				backgroundGrid.Indices32[k + 3] = i * n + j + 1;

				backgroundGrid.Indices32[k + 4] = i * n + j + 1;
				backgroundGrid.Indices32[k + 5] = (i + 1)*n + j + 1;

				backgroundGrid.Indices32[k + 6] = (i + 1)*n + j + 1;
				backgroundGrid.Indices32[k + 7] = (i + 1)*n + j;

				k += 8; // next quad
			}
		}
	}

	GeometryGenerator::MeshData grid = geoGen.CreateGrid(20.0f, 30.0f, 60, 40);
	GeometryGenerator::MeshData sphere = geoGen.CreateGeosphere(0.5f, 3);
	GeometryGenerator::MeshData cylinder = geoGen.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20);
	GeometryGenerator::MeshData skull = geoGen.CreateFromFile("skull.txt");
	GeometryGenerator::MeshData car = geoGen.CreateFromFile("car.txt");


	// Cache the vertex offsets to each object in the concatenated vertex buffer.
	UINT boxVertexOffset = 0;
	UINT backgroundGridVertexOffset = boxVertexOffset + (UINT)box.Vertices.size();
	UINT gridVertexOffset			= backgroundGridVertexOffset + (UINT)backgroundGrid.Vertices.size();
	UINT sphereVertexOffset			= gridVertexOffset + (UINT)grid.Vertices.size();
	UINT cylinderVertexOffset		= sphereVertexOffset + (UINT)sphere.Vertices.size();
	UINT skullVertexOffset			= cylinderVertexOffset + (UINT)cylinder.Vertices.size();
	UINT carVertexOffset			= skullVertexOffset + (UINT)skull.Vertices.size();

	// Cache the starting index for each object in the concatenated index buffer.
	UINT boxIndexOffset = 0;
	UINT backgroundGridIndexOffset	= boxIndexOffset + (UINT)box.Indices32.size();
	UINT gridIndexOffset			= backgroundGridIndexOffset+ (UINT)backgroundGrid.Indices32.size();
	UINT sphereIndexOffset			= gridIndexOffset + (UINT)grid.Indices32.size();
	UINT cylinderIndexOffset		= sphereIndexOffset + (UINT)sphere.Indices32.size();
	UINT skullIndexOffset			= cylinderIndexOffset + (UINT)cylinder.Indices32.size();
	UINT carIndexOffset				= skullIndexOffset + (UINT)skull.Indices32.size();

    // Define the SubmeshGeometry that cover different 
    // regions of the vertex/index buffers.

	SubmeshGeometry boxSubmesh;
	boxSubmesh.IndexCount = (UINT)box.Indices32.size();
	boxSubmesh.StartIndexLocation = boxIndexOffset;
	boxSubmesh.BaseVertexLocation = boxVertexOffset;

	SubmeshGeometry backgroundGridSubmesh;
	backgroundGridSubmesh.IndexCount = (UINT)backgroundGrid.Indices32.size();
	backgroundGridSubmesh.StartIndexLocation = backgroundGridIndexOffset;
	backgroundGridSubmesh.BaseVertexLocation = backgroundGridVertexOffset;

	SubmeshGeometry gridSubmesh;
	gridSubmesh.IndexCount = (UINT)grid.Indices32.size();
	gridSubmesh.StartIndexLocation = gridIndexOffset;
	gridSubmesh.BaseVertexLocation = gridVertexOffset;

	SubmeshGeometry sphereSubmesh;
	sphereSubmesh.IndexCount = (UINT)sphere.Indices32.size();
	sphereSubmesh.StartIndexLocation = sphereIndexOffset;
	sphereSubmesh.BaseVertexLocation = sphereVertexOffset;

	SubmeshGeometry cylinderSubmesh;
	cylinderSubmesh.IndexCount = (UINT)cylinder.Indices32.size();
	cylinderSubmesh.StartIndexLocation = cylinderIndexOffset;
	cylinderSubmesh.BaseVertexLocation = cylinderVertexOffset;

	SubmeshGeometry skullSubmesh;
	skullSubmesh.IndexCount = (UINT)skull.Indices32.size();
	skullSubmesh.StartIndexLocation = skullIndexOffset;
	skullSubmesh.BaseVertexLocation = skullVertexOffset;

	SubmeshGeometry carSubmesh;
	carSubmesh.IndexCount = (UINT)car.Indices32.size();
	carSubmesh.StartIndexLocation = carIndexOffset;
	carSubmesh.BaseVertexLocation = carVertexOffset;
	
	//
	// Extract the vertex elements we are interested in and pack the
	// vertices of all the meshes into one vertex buffer.
	//

	auto totalVertexCount =
		box.Vertices.size()
		+	backgroundGrid.Vertices.size() 
		+	grid.Vertices.size() 
		+	sphere.Vertices.size() 
		+	cylinder.Vertices.size()
		+	skull.Vertices.size()
		+	car.Vertices.size();

	std::vector<Vertex> vertices(totalVertexCount);

	UINT k = 0;
	for (size_t i = 0; i < box.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = box.Vertices[i].Position;
		vertices[k].Normal = box.Vertices[i].Normal;
	}

	for (size_t i = 0; i < backgroundGrid.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = backgroundGrid.Vertices[i].Position;
		vertices[k].Normal = backgroundGrid.Vertices[i].Normal;
	}

	for (size_t i = 0; i < grid.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = grid.Vertices[i].Position;
		vertices[k].Normal = grid.Vertices[i].Normal;
	}

	for (size_t i = 0; i < sphere.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = sphere.Vertices[i].Position;
		vertices[k].Normal = sphere.Vertices[i].Normal;
	}

	for (size_t i = 0; i < cylinder.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = cylinder.Vertices[i].Position;
		vertices[k].Normal = cylinder.Vertices[i].Normal;
	}

	for (size_t i = 0; i < skull.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = skull.Vertices[i].Position;
		vertices[k].Normal = skull.Vertices[i].Normal;
	}

	for (size_t i = 0; i < car.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = car.Vertices[i].Position;
		vertices[k].Normal = car.Vertices[i].Normal;
	}

	std::vector<std::uint16_t> indices;
	indices.insert(indices.end(), std::begin(box.GetIndices16()), std::end(box.GetIndices16()));
	indices.insert(indices.end(), std::begin(backgroundGrid.GetIndices16()), std::end(backgroundGrid.GetIndices16()));
	indices.insert(indices.end(), std::begin(grid.GetIndices16()), std::end(grid.GetIndices16()));
	indices.insert(indices.end(), std::begin(sphere.GetIndices16()), std::end(sphere.GetIndices16()));
	indices.insert(indices.end(), std::begin(cylinder.GetIndices16()), std::end(cylinder.GetIndices16()));
	indices.insert(indices.end(), std::begin(skull.GetIndices16()), std::end(skull.GetIndices16()));
	indices.insert(indices.end(), std::begin(car.GetIndices16()), std::end(car.GetIndices16()));

    const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
    const UINT ibByteSize = (UINT)indices.size()  * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "shapeGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	geo->DrawArgs["box"] = boxSubmesh;
	geo->DrawArgs["backgroundGrid"] = backgroundGridSubmesh;
	geo->DrawArgs["grid"] = gridSubmesh;
	geo->DrawArgs["sphere"] = sphereSubmesh;
	geo->DrawArgs["cylinder"] = cylinderSubmesh;
	geo->DrawArgs["skull"] = skullSubmesh;
	geo->DrawArgs["car"] = carSubmesh;

	mGeometries[geo->Name] = std::move(geo);
}

void TetrisApp::BuildMaterials()
{
	auto water = XMFLOAT3(0.2f, 0.2f, 0.2f);
	auto glass = XMFLOAT3(0.5f, 0.5f, 0.5f);
	auto plastic = XMFLOAT3(0.8f, 0.8f, 0.8f);

	auto roughness = 0.125f;

	int MatCBIndex = 0;
	int DiffuseSrvHeapIndex = 0;

	auto LightBlue = std::make_unique<Material>();
	LightBlue->Name = "Blue";
	LightBlue->MatCBIndex = MatCBIndex++;
	LightBlue->DiffuseSrvHeapIndex = DiffuseSrvHeapIndex++;
	LightBlue->DiffuseAlbedo = XMFLOAT4(Colors::LightBlue);
	LightBlue->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	//grass->FresnelR0 = water;
	//grass->FresnelR0 = glass;
	//grass->FresnelR0 = plastic;
	LightBlue->Roughness = roughness;

	auto DeepBlue = std::make_unique<Material>();
	DeepBlue->Name = "DeepBlue";
	DeepBlue->MatCBIndex = MatCBIndex++;
	DeepBlue->DiffuseSrvHeapIndex = DiffuseSrvHeapIndex++;
	DeepBlue->DiffuseAlbedo = XMFLOAT4(Colors::DeepSkyBlue);
	DeepBlue->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	//grass->FresnelR0 = water;
	//grass->FresnelR0 = glass;
	//grass->FresnelR0 = plastic;
	DeepBlue->Roughness = roughness;

	auto Orange = std::make_unique<Material>();
	Orange->Name = "Orange";
	Orange->MatCBIndex = MatCBIndex++;
	Orange->DiffuseSrvHeapIndex = DiffuseSrvHeapIndex++;
	Orange->DiffuseAlbedo = XMFLOAT4(Colors::Orange);
	Orange->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	//grass->FresnelR0 = water;
	//grass->FresnelR0 = glass;
	//grass->FresnelR0 = plastic;
	Orange->Roughness = roughness;

	auto Yellow = std::make_unique<Material>();
	Yellow->Name = "Yellow";
	Yellow->MatCBIndex = MatCBIndex++;
	Yellow->DiffuseSrvHeapIndex = DiffuseSrvHeapIndex++;
	Yellow->DiffuseAlbedo = XMFLOAT4(Colors::Yellow);
	Yellow->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	//grass->FresnelR0 = water;
	//grass->FresnelR0 = glass;
	//grass->FresnelR0 = plastic;
	Yellow->Roughness = roughness;

	auto Purple = std::make_unique<Material>();
	Purple->Name = "Purple";
	Purple->MatCBIndex = MatCBIndex++;
	Purple->DiffuseSrvHeapIndex = DiffuseSrvHeapIndex++;
	Purple->DiffuseAlbedo = XMFLOAT4(Colors::Purple);
	Purple->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	//grass->FresnelR0 = water;
	//grass->FresnelR0 = glass;
	//grass->FresnelR0 = plastic;
	Purple->Roughness = roughness;

	auto Green = std::make_unique<Material>();
	Green->Name = "Green";
	Green->MatCBIndex = MatCBIndex++;
	Green->DiffuseSrvHeapIndex = DiffuseSrvHeapIndex++;
	Green->DiffuseAlbedo = XMFLOAT4(Colors::LightGreen);
	Green->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	//grass->FresnelR0 = water;
	//grass->FresnelR0 = glass;
	//grass->FresnelR0 = plastic;
	Green->Roughness = roughness;

	auto Red = std::make_unique<Material>();
	Red->Name = "Red";
	Red->MatCBIndex = MatCBIndex++;
	Red->DiffuseSrvHeapIndex = DiffuseSrvHeapIndex++;
	Red->DiffuseAlbedo = XMFLOAT4(Colors::Red);
	Red->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	//grass->FresnelR0 = water;
	//grass->FresnelR0 = glass;
	//grass->FresnelR0 = plastic;
	Red->Roughness = roughness;

	auto White = std::make_unique<Material>();
	White->Name = "White";
	White->MatCBIndex = MatCBIndex++;
	White->DiffuseSrvHeapIndex = DiffuseSrvHeapIndex++;
	White->DiffuseAlbedo = XMFLOAT4(Colors::White);
	White->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	//grass->FresnelR0 = water;
	//grass->FresnelR0 = glass;
	//grass->FresnelR0 = plastic;
	White->Roughness = roughness;

	mMaterials["LightBlue"] = std::move(LightBlue);
	mMaterials["DeepBlue"] = std::move(DeepBlue);
	mMaterials["Orange"] = std::move(Orange);
	mMaterials["Yellow"] = std::move(Yellow);
	mMaterials["Purple"] = std::move(Purple);
	mMaterials["Green"] = std::move(Green);
	mMaterials["Red"] = std::move(Red);
	mMaterials["White"] = std::move(White);
}

void TetrisApp::BuildPSOs()
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;

	//
	// PSO for opaque objects.
	//
    ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	opaquePsoDesc.pRootSignature = mRootSignature.Get();
	opaquePsoDesc.VS = 
	{ 
		reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()), 
		mShaders["standardVS"]->GetBufferSize()
	};
	opaquePsoDesc.PS = 
	{ 
		reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()),
		mShaders["opaquePS"]->GetBufferSize()
	};
	opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaquePsoDesc.SampleMask = UINT_MAX;
	opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaquePsoDesc.NumRenderTargets = 1;
	opaquePsoDesc.RTVFormats[0] = mBackBufferFormat;
	opaquePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	opaquePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	opaquePsoDesc.DSVFormat = mDepthStencilFormat;
    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs["opaque"])));


    //
    // PSO for opaque wireframe objects.
    //

    D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueWireframePsoDesc = opaquePsoDesc;
    opaqueWireframePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaqueWireframePsoDesc, IID_PPV_ARGS(&mPSOs["opaque_wireframe"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueToonShadingPsoDesc = opaquePsoDesc;
	opaqueToonShadingPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["opaqueToonShadingPS"]->GetBufferPointer()),
		mShaders["opaqueToonShadingPS"]->GetBufferSize()
	};
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaqueToonShadingPsoDesc, IID_PPV_ARGS(&mPSOs["opaque_toonShading"])));
}

void TetrisApp::BuildFrameResources()
{
    for(int i = 0; i < gNumFrameResources; ++i)
    {
        mFrameResources.push_back(std::make_unique<FrameResource>(md3dDevice.Get(),
            1, (UINT)mMaterials.size(), (UINT)mAllRitems.size()));
    }
}

void TetrisApp::BuildRenderItemsOnMap()
{
	mObjCBIndex = 0;

	mAllRitems.clear();
	int x, y;
	for (y = 0; y < HEIGHT; y++) {
		for (x = 0; x < WIDTH; x++) {
			if (map[y][x] == 8) AddSkullRItem(x, y);
			if (map[y][x] < 8)	
				AddRenderItem(map[y][x], x, y);
		}
	}

	mRitemLayer[(int)RenderLayer::Opaque].clear();
	// All the render items are opaque.
	for(auto& e : mAllRitems)
		mRitemLayer[(int)RenderLayer::Opaque].push_back(e.get());
}

void TetrisApp::BuildbackgrounGrid()
{
	XMMATRIX world = XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixRotationX(-XM_PIDIV2) *XMMatrixTranslation(0.0f, 0.5f, 0.5f);
	
	auto backgroundGridRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&backgroundGridRitem->World, world);
	XMStoreFloat4x4(&backgroundGridRitem->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
	backgroundGridRitem->ObjCBIndex = mObjCBIndex++;
	backgroundGridRitem->Geo = mGeometries["shapeGeo"].get();
	backgroundGridRitem->Mat = mMaterials["Green"].get();
	backgroundGridRitem->Mat->NumFramesDirty = gNumFrameResources;
	backgroundGridRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
	backgroundGridRitem->IndexCount = backgroundGridRitem->Geo->DrawArgs["backgroundGrid"].IndexCount;
	backgroundGridRitem->StartIndexLocation = backgroundGridRitem->Geo->DrawArgs["backgroundGrid"].StartIndexLocation;
	backgroundGridRitem->BaseVertexLocation = backgroundGridRitem->Geo->DrawArgs["backgroundGrid"].BaseVertexLocation;
	mAllRitems.push_back(std::move(backgroundGridRitem));

	mRitemLayer[(int)RenderLayer::Opaque].push_back(mAllRitems.back().get());
}

void TetrisApp::BuildRenderItemsOnCBlock(){
	int i, j;
	for (i = 0; i < 5; i++) {
		for (j = 0; j < 5; j++) {
			if (CBlock[i][j] < 7)
				AddRenderItem(CBlock[i][j], x + j, y + i);
		}
	}
}

void TetrisApp::AddSkullRItem(int x, int y) {
	XMMATRIX world = XMMatrixScaling(0.2f, 0.2f, 0.2f) * XMMatrixRotationX(-XM_PIDIV4) * XMMatrixTranslation((float)x - WIDTH / 2, HEIGHT / 2 - (float)y, 0.5f);
	auto SkullRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&SkullRitem->World, world);
	XMStoreFloat4x4(&SkullRitem->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
	SkullRitem->ObjCBIndex = mObjCBIndex++;
	SkullRitem->Geo = mGeometries["shapeGeo"].get();
	SkullRitem->Mat = mMaterials["White"].get();
	SkullRitem->Mat->NumFramesDirty = gNumFrameResources;
	SkullRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	SkullRitem->IndexCount = SkullRitem->Geo->DrawArgs["skull"].IndexCount;
	SkullRitem->StartIndexLocation = SkullRitem->Geo->DrawArgs["skull"].StartIndexLocation;
	SkullRitem->BaseVertexLocation = SkullRitem->Geo->DrawArgs["skull"].BaseVertexLocation;
	mAllRitems.push_back(std::move(SkullRitem));

	mRitemLayer[(int)RenderLayer::Opaque].push_back(mAllRitems.back().get());
}

void TetrisApp::AddRenderItem(unsigned int type, int x, int y)
{
	XMMATRIX world = XMMatrixScaling(1.0f, 1.0f, 1.0f)*XMMatrixTranslation( (float)x - WIDTH/2, HEIGHT/2 - (float)y, 0.0f);

	std::string blockType;
	switch (type) {
		case 0:
			blockType = "LightBlue";
			break;
		case 1:
			blockType = "Purple";
			break;
		case 2:
			blockType = "DeepBlue";
			break;
		case 3:
			blockType = "Orange";
			break;
		case 4:
			blockType = "Yellow";
			break;
		case 5:
			blockType = "Green";
			break;
		case 6:
			blockType = "Red";
			break;
		case 7:
			blockType = "White";
			break;
	}

	auto newBoxRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&newBoxRitem->World, world);
	XMStoreFloat4x4(&newBoxRitem->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
	newBoxRitem->ObjCBIndex = mObjCBIndex++;
	newBoxRitem->Geo = mGeometries["shapeGeo"].get();
	newBoxRitem->Mat = mMaterials[blockType].get();
	newBoxRitem->Mat->NumFramesDirty = gNumFrameResources;
	newBoxRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	newBoxRitem->IndexCount = newBoxRitem->Geo->DrawArgs["box"].IndexCount;
	newBoxRitem->StartIndexLocation = newBoxRitem->Geo->DrawArgs["box"].StartIndexLocation;
	newBoxRitem->BaseVertexLocation = newBoxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
	mAllRitems.push_back(std::move(newBoxRitem));

	mRitemLayer[(int)RenderLayer::Opaque].push_back(mAllRitems.back().get());
}

void TetrisApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
{
    UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));
 
	auto objectCB = mCurrFrameResource->ObjectCB->Resource();
	auto matCB = mCurrFrameResource->MaterialCB->Resource();

    // For each render item...
    for(size_t i = 0; i < ritems.size(); ++i)
    {
        auto ri = ritems[i];

        cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
        cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
        cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

        // Offset to the CBV in the descriptor heap for this object and for this frame resource.
        UINT cbvIndex = mCurrFrameResourceIndex*(UINT)mRitemLayer[(int)RenderLayer::Opaque].size() + ri->ObjCBIndex;
        auto cbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvHeap->GetGPUDescriptorHandleForHeapStart());
        cbvHandle.Offset(cbvIndex, mCbvSrvUavDescriptorSize);

		D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress() + ri->Mat->MatCBIndex*matCBByteSize;

        cmdList->SetGraphicsRootDescriptorTable(0, cbvHandle);
		cmdList->SetGraphicsRootConstantBufferView(1, matCBAddress);

        cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
    }
}
