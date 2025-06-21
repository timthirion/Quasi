import Cocoa
import Metal
import MetalKit

class TriangleRenderer: NSObject, MTKViewDelegate {
    private var device: MTLDevice!
    private var commandQueue: MTLCommandQueue!
    private var renderPipelineState: MTLRenderPipelineState!
    private var vertexBuffer: MTLBuffer!
    
    struct Vertex {
        let position: SIMD3<Float>
        let color: SIMD3<Float>
    }
    
    private let vertices: [Vertex] = [
        Vertex(position: SIMD3<Float>(0.0, 0.5, 0.0), color: SIMD3<Float>(1.0, 0.0, 0.0)),   // Top - Red
        Vertex(position: SIMD3<Float>(-0.5, -0.5, 0.0), color: SIMD3<Float>(0.0, 1.0, 0.0)), // Bottom left - Green  
        Vertex(position: SIMD3<Float>(0.5, -0.5, 0.0), color: SIMD3<Float>(0.0, 0.0, 1.0))   // Bottom right - Blue
    ]
    
    init(device: MTLDevice) {
        self.device = device
        super.init()
        
        setupMetal()
    }
    
    private func setupMetal() {
        commandQueue = device.makeCommandQueue()
        
        // Create vertex buffer
        let vertexDataSize = vertices.count * MemoryLayout<Vertex>.size
        vertexBuffer = device.makeBuffer(bytes: vertices, length: vertexDataSize, options: [])
        
        // Create render pipeline using embedded shaders
        let source = """
        #include <metal_stdlib>
        using namespace metal;
        
        struct VertexIn {
            float3 position [[attribute(0)]];
            float3 color [[attribute(1)]];
        };
        
        struct VertexOut {
            float4 position [[position]];
            float3 color;
        };
        
        vertex VertexOut vertex_main(const device VertexIn* vertices [[buffer(0)]],
                                    uint vertexId [[vertex_id]]) {
            VertexOut out;
            VertexIn in = vertices[vertexId];
            
            out.position = float4(in.position, 1.0);
            out.color = in.color;
            
            return out;
        }
        
        fragment float4 fragment_main(VertexOut in [[stage_in]]) {
            return float4(in.color, 1.0);
        }
        """
        
        guard let library = try? device.makeLibrary(source: source, options: nil) else {
            fatalError("Failed to create Metal library")
        }
        
        let vertexFunction = library.makeFunction(name: "vertex_main")
        let fragmentFunction = library.makeFunction(name: "fragment_main")
        
        // Create vertex descriptor
        let vertexDescriptor = MTLVertexDescriptor()
        vertexDescriptor.attributes[0].format = .float3
        vertexDescriptor.attributes[0].offset = 0
        vertexDescriptor.attributes[0].bufferIndex = 0
        
        vertexDescriptor.attributes[1].format = .float3
        vertexDescriptor.attributes[1].offset = MemoryLayout<SIMD3<Float>>.size
        vertexDescriptor.attributes[1].bufferIndex = 0
        
        vertexDescriptor.layouts[0].stride = MemoryLayout<Vertex>.size
        
        let pipelineDescriptor = MTLRenderPipelineDescriptor()
        pipelineDescriptor.vertexFunction = vertexFunction
        pipelineDescriptor.fragmentFunction = fragmentFunction
        pipelineDescriptor.vertexDescriptor = vertexDescriptor
        pipelineDescriptor.colorAttachments[0].pixelFormat = .bgra8Unorm
        
        do {
            renderPipelineState = try device.makeRenderPipelineState(descriptor: pipelineDescriptor)
        } catch {
            fatalError("Failed to create render pipeline state: \(error)")
        }
    }
    
    func mtkView(_ view: MTKView, drawableSizeWillChange size: CGSize) {
        // Handle resize if needed
    }
    
    func draw(in view: MTKView) {
        guard let drawable = view.currentDrawable,
              let renderPassDescriptor = view.currentRenderPassDescriptor else {
            return
        }
        
        let commandBuffer = commandQueue.makeCommandBuffer()!
        let renderEncoder = commandBuffer.makeRenderCommandEncoder(descriptor: renderPassDescriptor)!
        
        renderEncoder.setRenderPipelineState(renderPipelineState)
        renderEncoder.setVertexBuffer(vertexBuffer, offset: 0, index: 0)
        renderEncoder.drawPrimitives(type: .triangle, vertexStart: 0, vertexCount: 3)
        renderEncoder.endEncoding()
        
        commandBuffer.present(drawable)
        commandBuffer.commit()
    }
}

class AppDelegate: NSObject, NSApplicationDelegate {
    var window: NSWindow!
    var metalView: MTKView!
    var renderer: TriangleRenderer!
    
    func applicationDidFinishLaunching(_ aNotification: Notification) {
        // Create window
        let windowRect = NSRect(x: 100, y: 100, width: 800, height: 600)
        window = NSWindow(contentRect: windowRect,
                         styleMask: [.titled, .closable, .resizable],
                         backing: .buffered,
                         defer: false)
        window.title = "Metal Triangle"
        window.center()
        
        // Setup Metal view
        guard let device = MTLCreateSystemDefaultDevice() else {
            fatalError("Metal is not supported on this device")
        }
        
        metalView = MTKView(frame: windowRect, device: device)
        metalView.clearColor = MTLClearColor(red: 0.0, green: 0.0, blue: 0.0, alpha: 1.0)
        
        renderer = TriangleRenderer(device: device)
        metalView.delegate = renderer
        
        window.contentView = metalView
        window.makeKeyAndOrderFront(nil)
        
        NSApp.activate(ignoringOtherApps: true)
    }
    
    func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool {
        return true
    }
}

// Create and run the app
let app = NSApplication.shared
let delegate = AppDelegate()
app.delegate = delegate
app.run()