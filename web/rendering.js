//
// Rendering functions.
//

import * as THREE from "three";
import { Line2 } from "three/addons/lines/Line2.js";
import { LineGeometry } from "three/addons/lines/LineGeometry.js";

import { getHitBoundaries } from "./helpers.js";
import { twoDXMat, twoDYMat, threeDGeoMat } from "./constants.js";

export function drawBox(hitType, group, hits, box) {
  if (hitType === "2D") return drawTwoDBoxVolume(group, hits);
  if (hitType === "3D") return drawBoxVolume(group, box);
}

// Given a box based detector volume, draw its wireframe.
export function drawBoxVolume(group, box) {
  const boxGeometry = new THREE.BoxGeometry(box.xWidth, box.yWidth, box.zWidth);
  const boxEdges = new THREE.EdgesGeometry(boxGeometry);
  const boxLines = new THREE.LineSegments(boxEdges, threeDGeoMat);

  boxLines.position.set(box.x, box.y, box.z);
  boxLines.updateMatrixWorld();

  group.add(boxLines);
}

// Given a box based detector volume, draw 2D axes.
export function drawTwoDBoxVolume(group, hits) {
  const createLine = (points, material) => {
    const axesGeo = new LineGeometry().setPositions(points);
    const axes = new Line2(axesGeo, material);
    axes.computeLineDistances();
    axes.scale.set(1, 1, 1);
    return axes;
  };

  const xProps = getHitBoundaries(hits, "x");
  const yProps = getHitBoundaries(hits, "y");

  const xPoints = [xProps.min, yProps.min, 0.0, xProps.max, yProps.min, 0.0];
  const yPoints = [xProps.min, yProps.min, 0.0, xProps.min, yProps.max, 0.0];

  const xAxes = createLine(xPoints, twoDXMat);
  const yAxes = createLine(yPoints, twoDYMat);

  group.add(xAxes, yAxes);
}

// Actual rendering animation function, called each frame.
export function animate(renderer, states, stats) {
  requestAnimationFrame(() => animate(renderer, states, stats));
  states.forEach((state) => {
    if (!state.visible) return;
    renderer.render(state.scene, state.camera);
    state.scene.matrixAutoUpdate = false;
    state.scene.autoUpdate = false;
  });
  stats.update();
}

// Given a geometry object, fit the camera to the center of it, such that the
// whole object can be seen.
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

  if (cameraType == "3D") {
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
    const yOffset = 0 - center.y / 2 - 25;
    camera.setViewOffset(
      window.innerWidth,
      window.innerHeight,
      0.0,
      yOffset,
      window.innerWidth,
      window.innerHeight,
    );
    const zoomAmount =
      Math.min(
        window.innerWidth / (boundingBox.max.x - boundingBox.min.x),
        window.innerHeight / (boundingBox.max.y - boundingBox.min.y),
      ) * 0.9;
    camera.zoom = zoomAmount;
  }

  // Update the camera + controls with these new parameters.
  controls.saveState();
  controls.update();
  camera.updateProjectionMatrix();
  camera.updateMatrix();
}
