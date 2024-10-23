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
    let cameraZ = Math.abs((maxDim / 4) * Math.tan(cameraFOV * 2));

    // Zoom out a bit, according to the padding factor...
    cameraZ *= offset;
    camera.position.z = cameraZ;

    // Apply limits to the camera...
    const minZ = boundingBox.min.z;
    const cameraToFarEdge = minZ < 0 ? -minZ + cameraZ : cameraZ - minZ;
    camera.far = cameraToFarEdge * 3;

    controls.target = center;
    controls.maxDistance = cameraToFarEdge;
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
