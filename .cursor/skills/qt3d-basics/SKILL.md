---
name: qt3d-basics
description: Sets up and builds minimal Qt3D applications in C++ (Qt5). Use when creating or modifying Qt3D C++ projects, adding a 3D view in C++, or when the user mentions Qt3D C++, Qt 3D, 3D scene, QEntity, Qt3DCore, Qt3DRender, Qt3DExtras, or embedding 3D in Qt Widgets.
---

# Qt3D Basics (C++)

Minimal setup and patterns for Qt3D applications in **C++ only** (Qt5). CMake, scene graph, Qt3DWindow, and embedding in QWidget.

## CMake (Qt5)

Required components and link:

```cmake
find_package(Qt5 REQUIRED COMPONENTS
  Core
  Gui
  Widgets
  3DCore
  3DRender
  3DExtras
  3DInput
  3DLogic
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  Qt5::Core
  Qt5::Gui
  Qt5::Widgets
  Qt5::3DCore
  Qt5::3DRender
  Qt5::3DExtras
  Qt5::3DInput
  Qt5::3DLogic
)
```

## Scene graph (C++)

- **Root**: One root `Qt3DCore::QEntity` (no parent). All other entities are its descendants.
- **Camera**: `Qt3DRender::QCamera` on an entity; set `projectionType`, `position`, `viewCenter`, `upVector` (or use `lens()` for near/far, FOV).
- **Mesh**: `Qt3DExtras::QSphereMesh`, `QCuboidMesh`, `QCylinderMesh`, etc., or `Qt3DRender::QMesh` for loaded models.
- **Material**: `Qt3DExtras::QPhongMaterial`, `QDiffuseSpecularMaterial`, etc. Add `Qt3DRender::QMaterial` component to the same entity as the mesh.
- **FrameGraph**: Use `Qt3DRender::QForwardRenderer` (or custom renderer) and set it on `Qt3DRender::QRenderSettings` attached to the root entity.

## Minimal C++ window (Qt3DWindow)

```cpp
#include <Qt3DExtras/Qt3DWindow>
#include <Qt3DCore/QEntity>
#include <Qt3DRender/QCamera>
#include <Qt3DRender/QRenderSettings>
#include <Qt3DExtras/QForwardRenderer>
#include <Qt3DExtras/QSphereMesh>
#include <Qt3DExtras/QPhongMaterial>

// Root entity (no parent)
Qt3DCore::QEntity *root = new Qt3DCore::QEntity;

// Camera entity (create first so it can be set on the renderer)
Qt3DCore::QEntity *cameraEntity = new Qt3DCore::QEntity(root);
Qt3DRender::QCamera *camera = new Qt3DRender::QCamera(cameraEntity);
camera->setPosition(QVector3D(0, 5, 0));
camera->setViewCenter(QVector3D(0, 0, 0));
cameraEntity->addComponent(camera);

// Renderer
Qt3DRender::QRenderSettings *renderSettings = new Qt3DRender::QRenderSettings(root);
Qt3DExtras::QForwardRenderer *forwardRenderer = new Qt3DExtras::QForwardRenderer;
forwardRenderer->setCamera(camera);
renderSettings->setActiveFrameGraph(forwardRenderer);
root->addComponent(renderSettings);

// Object entity (e.g. sphere)
Qt3DCore::QEntity *sphereEntity = new Qt3DCore::QEntity(root);
sphereEntity->addComponent(new Qt3DExtras::QSphereMesh);
sphereEntity->addComponent(new Qt3DExtras::QPhongMaterial);
```

Then create `Qt3DExtras::Qt3DWindow`, set its root entity to `root`, and show it (or embed via `QWidget::createWindowContainer`).

## Embedding in a QWidget app

Use `QWidget::createWindowContainer(Qt3DWindow)` to put the 3D window in a layout. The root entity and frame graph setup are the same as above; only the window is embedded.

## Conventions (C++)

- Use **Qt5** C++ namespaces/classes: `Qt3DCore::QEntity`, `Qt3DRender::QCamera`, `Qt3DExtras::QSphereMesh`, `QPhongMaterial`, etc.
- All 3D objects live under one root `Qt3DCore::QEntity`; camera and frame graph are part of the same scene.
- Prefer `Qt3DExtras` for simple meshes and materials; use `Qt3DRender::QMesh` and custom materials when needed.
- This skill is C++ only; QML Qt3D is out of scope.
