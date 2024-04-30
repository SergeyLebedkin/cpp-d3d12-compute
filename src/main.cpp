#include <iostream>
#include <cassert>
#include <d3d12.h>
#include <d3dcompiler.h>

// compute shader image write
const char* computeShader_ImageWrite = R"(
    struct SolidColor { float4 color; };

    RWTexture2D<float4> inputImage  : register(u0);
    RWTexture2D<float4> outputImage : register(u1);
    SolidColor          uSolidColor : register(c0);

    [numthreads(8, 8, 1)]
    void main(uint2 id: SV_DispatchThreadID) {
        outputImage[id] = uSolidColor.color;
    }
)";

void CompileRootSigrature(ID3D12Device* device, const D3D12_ROOT_SIGNATURE_DESC* rootSignatureDesc, ID3D12RootSignature** rootSignature) {
    // create root signature blob
    ID3DBlob* rootSignatureBlob{};
    ID3DBlob* errorBlob{};
    D3D12SerializeRootSignature(rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSignatureBlob, &errorBlob);
    if (errorBlob) std::cout << (const char*)errorBlob->GetBufferPointer() << std::endl;
    assert(rootSignatureBlob);
    // create root signature
    device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(rootSignature));
    assert(rootSignature);
    rootSignatureBlob->Release();
}

void CompileComputeShader(std::string_view source, ID3DBlob** shader) {
    ID3DBlob* error{};
    D3DCompile(source.data(), source.length(), NULL, NULL, NULL, "main", "cs_5_0", 0, 0, shader, &error);
    if (error) std::cout << (const char*)(error->GetBufferPointer()) << std::endl;
    assert(shader);
}

int main(int argc, char** argv) {
    // create device
    ID3D12Device* device{};
    D3D12CreateDevice(NULL, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device));
    assert(device);

    // create root signature description
    D3D12_DESCRIPTOR_RANGE images[] = { { D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 2, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND } };
    D3D12_DESCRIPTOR_RANGE buffer[] = { { D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND } };
    D3D12_ROOT_PARAMETER rootParameters[] = { 
        { D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, 1, images, D3D12_SHADER_VISIBILITY_ALL },
        { D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, 1, buffer, D3D12_SHADER_VISIBILITY_ALL }
    };
    D3D12_ROOT_SIGNATURE_DESC rootSignDesc = { 2, rootParameters, 0, NULL, D3D12_ROOT_SIGNATURE_FLAG_NONE };
    // create compute shader root signature
    ID3D12RootSignature* computeShaderRootSign{};
    CompileRootSigrature(device, &rootSignDesc, &computeShaderRootSign);
    assert(computeShaderRootSign);

    // create shader blob
    ID3DBlob* computeShaderBlob{};
    CompileComputeShader(computeShader_ImageWrite, &computeShaderBlob);
    assert(computeShaderBlob);

    // create compute pipeline state
    D3D12_COMPUTE_PIPELINE_STATE_DESC computePipelineStateDesc{};
    computePipelineStateDesc.pRootSignature = computeShaderRootSign;
    computePipelineStateDesc.CS.pShaderBytecode = computeShaderBlob->GetBufferPointer();
    computePipelineStateDesc.CS.BytecodeLength = computeShaderBlob->GetBufferSize();
    computePipelineStateDesc.NodeMask = 0;
    computePipelineStateDesc.CachedPSO.pCachedBlob = NULL;
    computePipelineStateDesc.CachedPSO.CachedBlobSizeInBytes = 0;
    computePipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    ID3D12PipelineState* computePipelineState{};
    device->CreateComputePipelineState(&computePipelineStateDesc, IID_PPV_ARGS(&computePipelineState));
    assert(computePipelineState);

    // create uav texture
    D3D12_HEAP_PROPERTIES heapProperties{};
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProperties.CreationNodeMask = 1;
    heapProperties.VisibleNodeMask = 1;
    D3D12_RESOURCE_DESC resourceDesc{};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resourceDesc.Alignment = 0;
    resourceDesc.Width = 512;
    resourceDesc.Height = 512;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.SampleDesc.Quality = 0;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    ID3D12Resource* uavTex{};
    device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, NULL, IID_PPV_ARGS(&uavTex));
    assert(uavTex);

    // create read buffer
    D3D12_HEAP_PROPERTIES heapPropertiesRead{};
    heapPropertiesRead.Type = D3D12_HEAP_TYPE_READBACK;
    heapPropertiesRead.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapPropertiesRead.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapPropertiesRead.CreationNodeMask = 1;
    heapPropertiesRead.VisibleNodeMask = 1;
    D3D12_RESOURCE_DESC resourceReadDesc{};
    resourceReadDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceReadDesc.Alignment = 0;
    resourceReadDesc.Width = 512 * 512 * 4;
    resourceReadDesc.Height = 1;
    resourceReadDesc.DepthOrArraySize = 1;
    resourceReadDesc.MipLevels = 1;
    resourceReadDesc.Format = DXGI_FORMAT_UNKNOWN;
    resourceReadDesc.SampleDesc.Count = 1;
    resourceReadDesc.SampleDesc.Quality = 0;
    resourceReadDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resourceReadDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    ID3D12Resource* readBuffer{};
    device->CreateCommittedResource(&heapPropertiesRead, D3D12_HEAP_FLAG_NONE, &resourceReadDesc, D3D12_RESOURCE_STATE_COPY_DEST, NULL, IID_PPV_ARGS(&readBuffer));
    assert(readBuffer);

    // create uav tex view
    D3D12_DESCRIPTOR_HEAP_DESC uavTexHeapDesc{};
    uavTexHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    uavTexHeapDesc.NumDescriptors = 1;
    uavTexHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    uavTexHeapDesc.NodeMask = 0;
    ID3D12DescriptorHeap* uavTexHeap{};
    device->CreateDescriptorHeap(&uavTexHeapDesc, IID_PPV_ARGS(&uavTexHeap));
    assert(uavTexHeap);
    D3D12_CPU_DESCRIPTOR_HANDLE uavTexViewCpuHandle{};
    D3D12_GPU_DESCRIPTOR_HANDLE uavTexViewGpuHandle{};
    uavTexHeap->GetCPUDescriptorHandleForHeapStart(&uavTexViewCpuHandle);
    uavTexHeap->GetGPUDescriptorHandleForHeapStart(&uavTexViewGpuHandle);
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavTexDesc{};
    uavTexDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    uavTexDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    uavTexDesc.Texture2D.MipSlice = 0;
    uavTexDesc.Texture2D.PlaneSlice = 0;
    device->CreateUnorderedAccessView(uavTex, NULL, &uavTexDesc, uavTexViewCpuHandle);

    // create signal
    ID3D12Fence* fence{};
    device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
    assert(fence);
    // create event handle
    HANDLE eventHandle = CreateEventA(NULL, FALSE, FALSE, NULL);
    // create command queue
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
    commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    commandQueueDesc.Priority = 0;
    commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    commandQueueDesc.NodeMask = 0;
    ID3D12CommandQueue* commandQueue;
    device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue));
    assert(commandQueue);
    // create command allocator
    ID3D12CommandAllocator* commandAllocator{};
    device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
    assert(commandAllocator);
    commandAllocator->Reset();
    // create command list
    ID3D12GraphicsCommandList* commandList{};
    device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, NULL, IID_PPV_ARGS(&commandList));
    assert(commandList);
    commandList->Close();

    // ********************************************************
    // Fill command list
    // Execute command list
    // ???
    // Profit!
    // ********************************************************

    // release handles
    commandList->Release();
    commandAllocator->Release();
    commandQueue->Release();
    CloseHandle(eventHandle);
    fence->Release();
    uavTex->Release();
    uavTexHeap->Release();
    computePipelineState->Release();
    computeShaderBlob->Release();
    computeShaderRootSign->Release();
    device->Release();

    return 0;
}
