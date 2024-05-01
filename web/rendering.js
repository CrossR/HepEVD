//
// Rendering functions.
//

import * as THREE from "three";
import { Line2 } from "three/addons/lines/Line2.js";
import { LineGeometry } from "three/addons/lines/LineGeometry.js";
import { ConvexGeometry } from "three/addons/geometries/ConvexGeometry.js";
import { Lut } from "three/addons/math/Lut.js";

import { threeDGeoMat, twoDXMat, twoDYMat } from "./constants.js";
import { getHitBoundaries } from "./helpers.js";
import { draw2DScaleBar } from "./markers.js";
import { addColourMap } from "./colourmaps.js";

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

  if (hits.length === 0) return;

  const xProps = getHitBoundaries(hits, "x");
  const yProps = getHitBoundaries(hits, "y");

  const xPoints = [xProps.min, yProps.min, 0.0, xProps.max, yProps.min, 0.0];
  const yPoints = [xProps.min, yProps.min, 0.0, xProps.min, yProps.max, 0.0];

  const xAxes = createLine(xPoints, twoDXMat);
  const yAxes = createLine(yPoints, twoDYMat);

  group.add(xAxes, yAxes);
}

/**
 * Draw trapezoids in 3D space, when given the 4 input points.
 *
 * @param {THREE.Group} group - The group to add the trapezoid to.
 * @param {Array} trapezoids - The trapezoids to draw.
 */
export function drawTrapezoids(group, trapezoids) {
  // Two passes are needed:
  //  - Find all trapezoids with the same or scaled measurements
  //  - Draw them all out at once with an InstancedMesh, to reduce draw calls.
  const meshes = new Map();

  trapezoids.forEach((trapezoid) => {
    const topLeft = trapezoid.topLeft;
    const bottomRight = trapezoid.bottomRight;
    const key = `${topLeft.x}-${topLeft.y}-${bottomRight.x}-${bottomRight.y}`;

    if (meshes.has(key)) meshes.get(key).push(trapezoid);
    else meshes.set(key, [trapezoid]);
  });

  // Now, draw out all the trapezoids.
  // Make a geometry based on the first object, then instanced mesh the rest.
  meshes.forEach((traps) => {
    const base = traps[0];
    const topLeft = base.topLeft;
    const topRight = base.topRight;
    const bottomLeft = base.bottomLeft;
    const bottomRight = base.bottomRight;

    const geometry = new ConvexGeometry([
      new THREE.Vector3(topLeft.x, topLeft.y, 0),
      new THREE.Vector3(topRight.x, topRight.y, 0),
      new THREE.Vector3(bottomRight.x, bottomRight.y, 0),
      new THREE.Vector3(bottomLeft.x, bottomLeft.y, 0),
    ]);

    const testMat = new THREE.LineBasicMaterial({
      side: THREE.DoubleSide,
    });
    const mesh = new THREE.InstancedMesh(geometry, testMat, traps.length);
    const dummyObject = new THREE.Object3D();

    traps.forEach((trapezoid, index) => {
      // The trapezoids may all have the same geometry, but they are not
      // in the same position. We need to update the matrix for each one...
      const pos = trapezoid.position;
      dummyObject.position.set(pos.x, pos.y, pos.z);
      dummyObject.updateMatrix();

      mesh.setMatrixAt(index, dummyObject.matrix);
    });

    mesh.instanceMatrix.needsUpdate = true;
    group.add(mesh);
  });
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
    draw2DScaleBar(state);
  });
  stats.update();
}

/**
 * Updates the camera aspect ratio and renderer size when the window is resized.
 *
 * @param {RenderState} state - The render state to update
 * @param {THREE.WebGLRenderer} renderer - The renderer to update.
 */
export function onWindowResize(state, renderer) {
  if (state.camera instanceof THREE.PerspectiveCamera) {
    state.camera.aspect = window.innerWidth / window.innerHeight;
  } else {
    state.camera.left = window.innerWidth / -2;
    state.camera.right = window.innerWidth / 2;
    state.camera.top = window.innerHeight / 2;
    state.camera.bottom = window.innerHeight / -2;
  }

  state.camera.updateProjectionMatrix();
  renderer.setSize(window.innerWidth, window.innerHeight);
  state.triggerEvent("change");
}
