#pragma once

namespace Ludus::Graphics
{
	struct Camera;
}

namespace Ludus
{
    class Engine;

    // defines application work at each stage of the engine-owned lifecycle
    class IApplication
    {
    public:
        virtual ~IApplication() = default;

        // runs once after the window, assets, and renderer are available
        virtual bool OnInitialize(Engine&) { return true; }

        // can run zero or more times before OnUpdate to maintain a fixed step
        virtual void OnFixedUpdate(Engine&, double) {}

        // runs once per frame before RenderEngine consumes submitted work
        virtual void OnUpdate(Engine&) {}

        // records UI only after world rendering starts successfully
        virtual void OnImGui(Engine&) {}

        // runs after every OnInitialize call, including failed initialization
        virtual void OnShutdown(Engine&) {}

        // supplies the camera used to render the current frame
		virtual void ConfigureCamera(Ludus::Graphics::Camera&) const {}
    };
}
