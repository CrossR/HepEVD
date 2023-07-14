//
// Rendering functions.
//

import * as THREE from "three";
import { Line2 } from "three/addons/lines/Line2.js";
import { LineGeometry } from "three/addons/lines/LineGeometry.js";

import { getHitBoundaries } from "./helpers.js";
import { twoDXMat, twoDYMat } from "./constants.js";

// Given a box based detector volume, draw its wireframe.
export function drawBoxVolume(group, material, box) {
  const boxGeometry = new THREE.BoxGeometry(box.xWidth, box.yWidth, box.zWidth);
  const boxEdges = new THREE.EdgesGeometry(boxGeometry);
  const boxLines = new THREE.LineSegments(boxEdges, material);

  boxLines.position.set(box.x, box.y, box.z);
  boxLines.updateMatrixWorld();

  group.add(boxLines);
}

// Given a box based detector volume, draw 2D axes.
export function drawTwoDBoxVolume(hits, hitGroup, group) {

  const xProps = getHitBoundaries(hits, "x");
  const xPoints = [xProps.min, 0.0, 0.0, xProps.max, 0.0, 0.0];

  const xAxesGeo = new LineGeometry().setPositions(xPoints);
  const xAxes = new Line2(xAxesGeo, twoDXMat);
  xAxes.computeLineDistances();
  xAxes.scale.set(1,1,1);
  group.add(xAxes);

  // Repeat for Y axes, but the line is drawn at the maxX value.
  const yProps = getHitBoundaries(hits, "y");
  const yPoints = [xProps.min, yProps.min, 0.0, xProps.min, yProps.max, 0.0];

  const yAxesGeo = new LineGeometry().setPositions(yPoints);
  const yAxes = new Line2(yAxesGeo, twoDYMat);
  yAxes.computeLineDistances();
  yAxes.scale.set(1,1,1);
  group.add(yAxes);
}

// Actual rendering animation function, called each frame.
export function animate(renderer, scenes, cameras, stats) {
  requestAnimationFrame(() => animate(renderer, scenes, cameras, stats));
  scenes.forEach((scene, hitType) => {
    if (!scene.visible) return;
    renderer.render(scene, cameras.get(hitType));
    scene.matrixAutoUpdate = false;
    scene.autoUpdate = false;
  });
  stats.update();
}

// Given a geometry object, fit the camera to the center of it, such that the
// whole object can be seen.
export function fitSceneInCamera(camera, controls, detectorGeometry, cameraType) {
  const offset = 1.5; // Padding factor.

  // Get the bounding box of the detector geometry.
  // This should be the group for best results.
  let boundingBox = new THREE.Box3().setFromObject(detectorGeometry);

  const size = boundingBox.getSize(new THREE.Vector3());
  const center = boundingBox.getCenter(new THREE.Vector3());

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

  // Update the camera with this new zoom position.
  camera.updateProjectionMatrix();

  // And if required, update the controls.
  if (controls) {
    controls.target = center;
    controls.maxDistance = cameraToFarEdge;
    controls.saveState();
  } else {
    camera.lookAt(center);
  }
}

// Show the correct scene based on the current render target (2D/3D).
export function toggleScene(scenes, controls, renderTarget) {
  ["2D", "3D"].forEach((hitType) => {
    scenes.get(hitType).visible = hitType === renderTarget;
    controls.get(hitType).enabled = hitType === renderTarget;
  });
}

// Show the correct scene based on the current render target (2D/3D).
export function isSceneActive(scenes, renderTarget) {
  return scenes.get(renderTarget).visible;
}
