//
// Rendering functions.
//

import * as THREE from "three";
import { ConvexGeometry } from "three/addons/geometries/ConvexGeometry.js";
import { Lut } from "three/addons/math/Lut.js";
import * as BufferGeometryUtils from "three/addons/utils/BufferGeometryUtils.js";

import { addColourMap, getContinuousLutConf } from "./colourmaps.js";
import { threeDGeoMat, threeDTrapezoidMat } from "./constants.js";
import { draw2DScaleBar } from "./markers.js";

/**
 * Draws a box based on the hit type.
 *
 * @param {string} hitType - The type of hit, either "2D" or "3D".
 * @param {THREE.Group} group - The group to add the box to.
 * @param {Object} box - The box to draw.
 */
export function drawBox(hitType, group, box) {
  if (hitType === "2D") {
    // Flips the y and z dimensions of the box, to draw it correctly in 2D.
    const flippedBox = {
      ...box,
      zWidth: box.yWidth,
      yWidth: box.zWidth,
    };
    flippedBox.position = {
      ...flippedBox.position,
      y: flippedBox.position.z,
      z: flippedBox.position.y,
    };
    return drawBoxVolume(group, flippedBox);
  }
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
 * Draw trapezoids in 3D space, when given the 4 input points.
 *
 * @param {THREE.Group} group - The group to add the trapezoid to.
 * @param {Array} trapezoids - The trapezoids to draw.
 */
export function drawTrapezoids(group, trapezoids) {
  if (trapezoids.length === 0) return;

  const meshes = new Map();

  // First, find all the trapezoids that share the same geometry.
  // Can do that by checking all the points, and calculating a key
  // based on the height and width of the trapezoid.
  trapezoids.forEach((trapezoid) => {
    const topLeft = trapezoid.topLeft;
    const bottomRight = trapezoid.bottomRight;
    const key = `${topLeft.x}-${topLeft.y}-${bottomRight.x}-${bottomRight.y}`;

    if (!meshes.has(key)) meshes.set(key, []);
    meshes.get(key).push(trapezoid);
  });

  const getVector = (point) => {
    return new THREE.Vector3(point.x, point.y, point.z);
  };

  // The final rendered result is a single, merged, BufferGeometry.
  const geometries = [];

  // Now, draw out all the trapezoids.
  // Make a geometry based on the first object, then instanced mesh the rest.
  meshes.forEach((traps) => {
    const base = traps[0];
    const basePos = base.position;
    const topLeft = base.topLeft;
    const topRight = base.topRight;
    const bottomLeft = base.bottomLeft;
    const bottomRight = base.bottomRight;

    const geometry = new ConvexGeometry([
      getVector(topLeft),
      getVector(topRight),
      getVector(bottomRight),
      getVector(bottomLeft),
    ]);

    const mesh = new THREE.InstancedMesh(
      geometry,
      threeDTrapezoidMat,
      traps.length,
    );

    traps.forEach((trapezoid, index) => {
      // The trapezoids may all have the same geometry, but they are not
      // in the same position. We need to update the matrix for each one...
      // We can do this by setting the position of the trapezoid to the
      // position of the base trapezoid, then adding the offset.
      const pos = trapezoid.position;
      const xOffset = pos.x - basePos.x;
      const yOffset = pos.y - basePos.y;
      const zOffset = pos.z - basePos.z;
      const offset = new THREE.Matrix4().makeTranslation(
        xOffset,
        yOffset,
        zOffset,
      );

      mesh.setMatrixAt(index, offset);

      const currentMatrix = new THREE.Matrix4();
      mesh.getMatrixAt(index, currentMatrix);

      const geoClone = geometry.clone();
      geoClone.applyMatrix4(currentMatrix);
      geometries.push(geoClone);
    });

    mesh.instanceMatrix.needsUpdate = true;
  });

  // Finally, merge and add to group.
  const mergedGeo = BufferGeometryUtils.mergeGeometries(geometries);
  const edges = new THREE.EdgesGeometry(mergedGeo);
  const line = new THREE.LineSegments(edges, threeDTrapezoidMat);
  group.add(line);
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
    console.log(`There was ${renderer.info.render.calls} render calls...`);
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

/**
 * Renders an image on the screen.
 *
 * @param {Object} image - The image to render
 * @return {HTMLImageElement} description of return value
 */
export function renderImage(image) {
  const dataValues = [...new Set(image.data.flat())];
  const maxValue = Math.max(...dataValues);
  const minValue = Math.min(...dataValues);

  const canvas = document.createElement("canvas");
  const ctx = canvas.getContext("2d");
  canvas.width = image.width;
  canvas.height = image.height;
  const imageData = ctx.createImageData(image.width, image.height);

  const hitColours = image.data;

  if (image.imageType === "Monochrome") {
    const colourLut = new Lut("cooltowarm", 10);
    const lutConfig = getContinuousLutConf();
    addColourMap(colourLut, lutConfig.name, lutConfig.size);

    colourLut.setMax(maxValue);
    colourLut.setMin(minValue);

    for (let row = 0; row < image.height; row++) {
      for (let col = 0; col < image.width; col++) {
        if (hitColours[row][col] !== 0)
          hitColours[row][col] = colourLut.getColor(hitColours[row][col]);
      }
    }
  }

  // Check if its a THREE.Color and scale between 0 - 255.
  // Finally if its just a raw RGB value, just use it.
  const getColour = (colour) => {
    if (colour instanceof THREE.Color) {
      return [255 * colour.r, 255 * colour.g, 255 * colour.b, 255];
    } else if (Array.isArray(colour) && colour.length === 3) {
      return [...colour, 255];
    } else {
      return [0.0, 0.0, 0.0, 0.0];
    }
  };

  for (let i = 0; i < imageData.data.length; i += 4) {
    const row = Math.floor(i / (image.height * 4));
    const col = Math.floor(i / 4) % image.width;
    const colour = getColour(hitColours[row][col]);

    // If the value isn't 0, scale every channel between 0 - 255, inverted.
    imageData.data[i + 0] = colour[0];
    imageData.data[i + 1] = colour[1];
    imageData.data[i + 2] = colour[2];
    imageData.data[i + 3] = colour[3];
  }

  ctx.putImageData(imageData, 0, 0);

  const im = new Image();
  im.src = canvas.toDataURL("image/png");
  im.style.width = `${image.width}px`;
  im.style.minWidth = `${image.width}px`;
  im.style.height = `${image.height}px`;
  im.style.minHeight = `${image.height}px`;

  return im;
}
