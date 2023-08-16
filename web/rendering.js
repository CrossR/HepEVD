//
// Rendering functions.
//

import * as THREE from "three";
import { Line2 } from "three/addons/lines/Line2.js";
import { LineGeometry } from "three/addons/lines/LineGeometry.js";

import { getHitBoundaries } from "./helpers.js";
import { twoDXMat, twoDYMat, threeDGeoMat } from "./constants.js";

/**
 * Draws a box based on the hit type.
 *
 * @param {string} hitType - The type of hit, either "2D" or "3D".
 * @param {THREE.Group} group - The group to add the box to.
 * @param {Array} hits - The hits to draw.
 * @param {Object} box - The box to draw.
 */
export function drawBox(hitType, group, hits, box) {
  if (hitType === "2D") return drawTwoDBoxVolume(group, hits);
  if (hitType === "3D") return drawBoxVolume(group, box);
}

/**
 * Draws a box volume in 3D space.
 *
 * @param {THREE.Group} group - The group to add the box to.
 * @param {Object} box - The box to draw.
 */
export function drawBoxVolume(group, box) {
  const boxGeometry = new THREE.BoxGeometry(box.xWidth, box.yWidth, box.zWidth);
  const boxEdges = new THREE.EdgesGeometry(boxGeometry);
  const boxLines = new THREE.LineSegments(boxEdges, threeDGeoMat);

  const boxPos = box.position;
  boxLines.position.set(boxPos.x, boxPos.y, boxPos.z);
  boxLines.updateMatrixWorld();

  group.add(boxLines);
}

/**
 * Draws a box volume in 2D space. In this case, the box is calculated based on
 * the input hits.
 *
 * @param {THREE.Group} group - The group to add the box to.
 * @param {Object} box - The box to draw.
 */
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

/**
 * Animates the renderer with the given states and updates the stats.
 *
 * @param {THREE.WebGLRenderer} renderer - The renderer to animate.
 * @param {Array} states - The states to animate.
 * @param {Stats} stats - The stats to update.
 */
export function animate(renderer, states, stats) {
  states.forEach((state) => {
    if (!state.visible) return;
    renderer.render(state.scene, state.camera);
    state.scene.matrixAutoUpdate = false;
    state.scene.autoUpdate = false;
  });
  stats.update();
}

/**
 * Updates the camera aspect ratio and renderer size when the window is resized.
 *
 * @param {THREE.Camera} camera - The camera to update.
 * @param {THREE.WebGLRenderer} renderer - The renderer to update.
 */
export function onWindowResize(camera, renderer) {
  if (camera instanceof THREE.PerspectiveCamera) {
    camera.aspect = window.innerWidth / window.innerHeight;
  } else {
    camera.left = window.innerWidth / -2;
    camera.right = window.innerWidth / 2;
    camera.top = window.innerHeight / 2;
    camera.bottom = window.innerHeight / -2;
  }
  camera.updateProjectionMatrix();

  renderer.setSize(window.innerWidth, window.innerHeight);
}
