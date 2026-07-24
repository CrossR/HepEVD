//
// Hit Rendering functions.
//

import * as THREE from "three";
import { OrbitControls } from "three/addons/controls/OrbitControls.js";

/**
 * Sets up the controls for the given view type.
 *
 * @param {string} viewType - The type of view ("2D" or "3D").
 * @param {Object} controls - The controls object to be set up.
 */
export function setupControls(viewType, controls) {
  if (viewType === "3D") setupThreeDControls(controls);
  else setupTwoDControls(controls);
}

/**
 * Sets up the controls for a 2D view.
 *
 * @param {OrbitControls} controls - The controls object to be set up.
 */
export function setupTwoDControls(controls) {
  controls.screenSpacePanning = true;
  controls.enableRotate = false;
  controls.mouseButtons = {
    LEFT: THREE.MOUSE.PAN,
    MIDDLE: THREE.MOUSE.DOLLY,
    RIGHT: null,
  };
  controls.touches = {
    ONE: THREE.TOUCH.PAN,
    TWO: THREE.TOUCH.DOLLY_ROTATE,
  };

  controls.update();
}

/**
 * Sets up the controls for a 3D view.
 *
 * @param {OrbitControls} controls - The controls object to be set up.
 */
export function setupThreeDControls(controls) {
  controls.screenSpacePanning = true;
  controls.enableRotate = true;
  controls.mouseButtons = {
    LEFT: THREE.MOUSE.ROTATE,
    MIDDLE: THREE.MOUSE.DOLLY,
    RIGHT: THREE.MOUSE.PAN,
  };

  controls.update();
}

/**
 * Adjusts the camera position and zoom to fit the entire scene in view.
 *
 * @param {THREE.PerspectiveCamera} camera - The camera to adjust.
 * @param {OrbitControls} controls - The controls object to use for the camera.
 * @param {THREE.Object3D} scene - The scene to fit in view.
 * @param {string} cameraType - The type of camera ("2D" or "3D").
 */
export function fitSceneInCamera(
  camera,
  controls,
  detectorGeometry,
  cameraType,
) {
  const offset = 1.5; // Padding factor.

  // Get the bounding box of the detector geometry.
  // This should be the group for best results.
  let boundingBox = new THREE.Box3().setFromObject(detectorGeometry);

  const size = boundingBox.getSize(new THREE.Vector3());
  const center = boundingBox.getCenter(new THREE.Vector3());

  if (cameraType === "3D") {
    // Get the maximum dimension of the bounding box...
    const maxDim = Math.max(size.x, size.y, size.z);
    const cameraFOV = camera.fov * (Math.PI / 180);

    // Calculate distance needed to fit the scene
    let cameraZ = maxDim / 2 / Math.tan(cameraFOV / 2);

    // Zoom out a bit, according to the padding factor...
    cameraZ *= offset;

    // Position camera at center, offset by calculated distance
    camera.position.set(center.x, center.y, center.z + cameraZ);

    // Apply limits to the camera...
    const minZ = boundingBox.min.z;
    const cameraToFarEdge = Math.abs(camera.position.z - minZ);
    camera.far = cameraToFarEdge * 3;
    camera.near = 0.1; // Make sure near plane is set

    controls.target.copy(center);
    controls.maxDistance = cameraToFarEdge * 2;
  } else {
    const yOffset = -center.y / 2 - 50;
    const xOffset = center.x;

    camera.setViewOffset(
      window.innerWidth,
      window.innerHeight,
      xOffset,
      yOffset,
      window.innerWidth,
      window.innerHeight,
    );
    const zoomAmount =
      Math.min(
        window.innerWidth / (boundingBox.max.x - boundingBox.min.x),
        window.innerHeight / (boundingBox.max.y - boundingBox.min.y),
      ) * 0.85;
    camera.zoom = zoomAmount;
  }

  // Update the camera + controls with these new parameters.
  controls.saveState();
  controls.update();
  camera.updateProjectionMatrix();
  camera.updateMatrix();
}

/**
 * Sets the camera projection to either 2D or 3D based on the specified plane.
 *
 * @param {Object} state - The current rendering state containing camera and controls.
 * @param {string} plane - The desired projection plane ("3D", "XY", "XZ", "YZ").
 */
export function setProjection(state, plane) {
  // If the user wants a 3D view, we can just restore the original
  // PerspectiveCamera.
  if (plane === "3D") {
    if (state.perspCamera) {
      state.camera = state.perspCamera;
      state.controls.object = state.camera;
      state.controls.enableRotate = true;
      fitSceneInCamera(state.camera, state.controls, state.detGeoGroup, "3D");
      state.triggerEvent("change");
    }
    return;
  }

  // Otherwise, we may need to create an orthographic camera if it doesn't exist
  // yet.
  if (!state.orthoCamera) {
    state.perspCamera = state.camera; // Save original PerspectiveCamera
    state.orthoCamera = new THREE.OrthographicCamera(
      window.innerWidth / -2, window.innerWidth / 2,
      window.innerHeight / 2, window.innerHeight / -2,
      -1e6, 1e6
    );
  }

  // Swap to orthographic
  state.camera = state.orthoCamera;
  state.controls.object = state.camera;
  state.controls.enableRotate = false; // Lock rotation for true 2D

  // Position the camera along the chosen axis
  const boundingBox = new THREE.Box3().setFromObject(state.detGeoGroup);
  const center = boundingBox.getCenter(new THREE.Vector3());
  const size = boundingBox.getSize(new THREE.Vector3());
  const distance = 10000; // Far enough to avoid clipping

  if (plane === 'XY') {
    state.camera.position.set(center.x, center.y, center.z + distance);
    state.camera.up.set(0, 1, 0);
  } else if (plane === 'XZ') {
    state.camera.position.set(center.x, center.y + distance, center.z);
    state.camera.up.set(0, 0, -1);
  } else if (plane === 'YZ') {
    state.camera.position.set(center.x + distance, center.y, center.z);
    state.camera.up.set(0, 1, 0);
  }

  state.controls.target.copy(center);

  // Calculate zoom to fit the geometry
  let width = plane === 'YZ' ? size.z : size.x;
  let height = plane === 'XZ' ? size.z : size.y;

  // Guard against division by zero on empty scenes
  width = width === 0 ? 1 : width * 1.2; // Add 20% padding
  height = height === 0 ? 1 : height * 1.2;

  const zoomX = window.innerWidth / width;
  const zoomY = window.innerHeight / height;
  state.camera.zoom = Math.min(zoomX, zoomY);

  state.camera.updateProjectionMatrix();
  state.controls.update();
  state.triggerEvent("change");
}
