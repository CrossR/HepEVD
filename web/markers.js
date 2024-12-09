//
// Marker Drawing
//

import * as THREE from "three";
import { Line2 } from "three/addons/lines/Line2.js";
import { LineGeometry } from "three/addons/lines/LineGeometry.js";
import { LineMaterial } from "three/addons/lines/LineMaterial.js";
import { Lut } from "three/addons/math/Lut.js";

import { MARKER_CONFIG } from "./constants.js";
import { getHitBoundaries } from "./helpers.js";
import { dragElement } from "./ui.js";

/**
 * Draws rings using the provided data and adds them to the specified group.
 * @param {Array} rings - An array of ring data objects.
 * @param {THREE.Group} group - The group to which the rings will be added.
 */
export function drawRingMarker(rings, group) {
  if (rings.length === 0) return;

  const bufferGeometry = new THREE.BufferGeometry();

  const vertexMap3D = new Map();
  const vertices = [];
  const indicies = [];
  const colors = [];

  const segments = 32;
  const startAngle = 0;
  const endAngle = Math.PI * 2;
  const theta = (endAngle - startAngle) / segments;

  let ringNumber = 0;

  rings.forEach((ring) => {
    if (ring.inner === 0) return;
    const innerRadius = ring.inner;
    const outerRadius = ring.outer;
    const x = ring.position.x;
    const y = ring.position.y;

    for (let i = 0; i <= segments; i++) {
      const angle = startAngle + i * theta;
      const cos = Math.cos(angle);
      const sin = Math.sin(angle);

      const x1 = x + cos * innerRadius;
      const y1 = y + sin * innerRadius;
      const x2 = x + cos * outerRadius;
      const y2 = y + sin * outerRadius;

      [
        [x1, y1],
        [x2, y2],
      ].forEach(([x, y]) => {
        // First, store the new vertex.
        // We store the vertices with decreasing z values, so that
        // there is less chance of z-fighting.
        const index = vertices.length / 3;
        vertices.push(x, y, ringNumber * -1);
        colors.push(1, 1, 1, 0.01);

        // Now, update the map.
        // Round to nearest five, so that we can group vertices together.
        // This isn't perfect, but it's good enough for now.
        const key = `${Math.round(x / 5) * 5},${Math.round(y / 5) * 5}`;
        if (vertexMap3D.has(key)) {
          vertexMap3D.get(key).push(index);
        } else {
          vertexMap3D.set(key, [index]);
        }
      });
    }

    const offset = vertices.length / 3 - (segments + 1) * 2;

    for (let i = 0; i < segments; i++) {
      [0, 1, 2, 2, 1, 3].forEach((posOffset) => {
        indicies.push(offset + i * 2 + posOffset);
      });
    }

    ringNumber += 1;
  });

  // Lets parse the vertexMap, to figure out a vertex score, that can be used
  // to color the vertices.
  const scores = [];
  vertexMap3D.forEach((indices, _key) => {
    const score = indices.length;
    scores.push(score);
  });
  const minScore = Math.min(...scores);
  const maxScore = Math.max(...scores);

  vertexMap3D.forEach((indices, key) => {
    const score = (indices.length - minScore) / (maxScore - minScore);

    // Now we know the colour, update the vertex with the lowest z value.
    const index = indices.forEach((index) => {
      [0, 1, 2].forEach((offset) => {
        colors[(index + offset) * 4 + 1] = 0.85 - score;
        colors[(index + offset) * 4 + 2] = 0.85 - score;

        colors[(index + offset) * 4 + 3] = 0.01 + score;
      });
    });
  });

  bufferGeometry.setAttribute(
    "position",
    new THREE.Float32BufferAttribute(vertices, 3),
  );
  bufferGeometry.setIndex(indicies);
  bufferGeometry.setAttribute(
    "color",
    new THREE.Float32BufferAttribute(colors, 4),
  );

  const ringMaterial = new THREE.MeshBasicMaterial({
    vertexColors: true,
    transparent: true,
  });

  const mesh = new THREE.Mesh(bufferGeometry, ringMaterial);
  mesh.matrixAutoUpdate = false;
  mesh.matrixWorldAutoUpdate = false;
  group.add(mesh);
}

/**
 * Draws points using the provided data and adds them to the specified group.
 * @param {Array} points - An array of point data objects.
 * @param {THREE.Group} group - The group to which the points will be added.
 */
export function drawPoints(points, group) {
  if (points.length === 0) return;

  let pointColours = [];

  points.forEach((point) => {
    // If the point has a colour, use that.
    if (point.colour) {
      pointColours.push(point.colour);
    } else {
      pointColours.push(MARKER_CONFIG["point"].colour);
    }
  });

  // Start building the mesh.
  const pointSize = MARKER_CONFIG["point"].size;
  const pointGeo = new THREE.SphereGeometry(pointSize, 32, 16);
  const materialPoint = new THREE.MeshBasicMaterial({
    side: THREE.DoubleSide,
    depthFunc: THREE.AlwaysDepth,
  });
  const dummyObject = new THREE.Object3D();
  const pointMesh = new THREE.InstancedMesh(
    pointGeo,
    materialPoint,
    points.length,
  );

  const lut = new Lut("cooltowarm", 512);
  let usingLut = typeof pointColours[0] === "number";

  if (usingLut) {
    let minColourValue = Infinity;
    let maxColourValue = Number.NEGATIVE_INFINITY;
    pointColours.forEach((value) => {
      if (value < minColourValue) minColourValue = value;
      if (value > maxColourValue) maxColourValue = value;
    });
    lut.setMax(maxColourValue);
  }

  points.forEach(function (point, index) {
    const pos = point.position;
    dummyObject.position.set(pos.x, pos.y, pos.z);
    dummyObject.updateMatrix();

    pointMesh.setMatrixAt(index, dummyObject.matrix);

    if (usingLut) {
      pointMesh.setColorAt(index, lut.getColor(pointColours[index]));
    } else {
      pointMesh.setColorAt(index, new THREE.Color(pointColours[index]));
    }
  });

  pointMesh.instanceMatrix.needsUpdate = true;
  pointMesh.instanceColor.needsUpdate = true;
  pointMesh.renderOrder = 999;

  group.add(pointMesh);
}

/**
 * Draws lines using the provided data and adds them to the specified group.
 * @param {Array} lines - An array of lines data objects.
 * @param {THREE.Group} group - The group to which the lines will be added.
 */
export function drawLines(lines, group) {
  if (lines.length === 0) return;

  let lineColours = [];

  lines.forEach((line) => {
    // If the line has a colour, use that.
    if (line.colour) {
      lineColours.push(line.colour);
    } else {
      lineColours.push(MARKER_CONFIG["line"].colour);
    }
  });

  const lut = new Lut("cooltowarm", 512);
  let usingLut = typeof lineColours[0] === "number";

  if (usingLut) {
    let minColourValue = Infinity;
    let maxColourValue = Number.NEGATIVE_INFINITY;
    pointColours.forEach((value) => {
      if (value < minColourValue) minColourValue = value;
      if (value > maxColourValue) maxColourValue = value;
    });
    lut.setMax(maxColourValue);
  }

  const lineMaterial = new LineMaterial({
    linewidth: MARKER_CONFIG["line"].size,
  });
  const lineObjects = [];

  lines.forEach(function (line, index) {
    const start = line.position;
    const end = line.end;
    const points = [start.x, start.y, start.z, end.x, end.y, end.z];

    if (usingLut) {
      lineMaterial.color = lut.getColor(lineColours[index]);
    } else {
      lineMaterial.color = new THREE.Color(lineColours[index]);
    }

    const lineGeo = new LineGeometry().setPositions(points);
    const lineObj = new Line2(lineGeo, lineMaterial);

    lineObj.computeLineDistances();
    lineObj.scale.set(1, 1, 1);

    lineObjects.push(lineObj);
  });

  group.add(...lineObjects);

  return;
}

/**
 * Draws a 2D scale bar on the screen based on the provided hits and camera.
 *
 * @param {RenderState} state - The render state to update
 */
export function draw2DScaleBar(state) {
  const scaleBar = document.getElementById("scale_bar");
  const scaleBarText = document.getElementById("scale_bar_text");

  // Don't bother for 3D.
  if (state.name !== "2D" && state.visible) {
    scaleBar.style.visibility = "hidden";
    scaleBarText.style.visibility = "hidden";
    return;
  } else if (state.name !== "2D") {
    return;
  }

  // First, get the X width of the detector.
  let hitsToUse = state.hitData.all;

  if (hitsToUse.length === 0 && state.particleData.all.length > 0) {
    hitsToUse = state.particleData.all.flatMap((particle) => particle.hits);
  }

  const xBoundary = getHitBoundaries(hitsToUse, "x");
  const xWidth = xBoundary.max - xBoundary.min;
  const xStart = 0 - xWidth / 2;
  const xEnd = xWidth / 2;

  // Project these points into the camera.
  const xStartVector = new THREE.Vector3(xStart, 0, 0).project(state.camera);
  const xEndVector = new THREE.Vector3(xEnd, 0, 0).project(state.camera);

  // Scale the points to the screen size.
  const xStartScaled = ((xStartVector.x + 1) / 2) * window.innerWidth;
  const xEndScaled = ((xEndVector.x + 1) / 2) * window.innerWidth;

  // Now, make a line that covers Xcm.
  const pixelPerCm = (xEndScaled - xStartScaled) / xWidth;
  const targetSizes = [1, 2, 5, 10, 20, 50, 100];
  const targetSize = targetSizes.find((size) => size * pixelPerCm > 100);
  const pixelsRequired = targetSize
    ? targetSize * pixelPerCm
    : 100 * pixelPerCm;

  // We know the pixels required, so we can draw a line.
  scaleBar.style.visibility = "visible";
  scaleBarText.style.visibility = "visible";
  scaleBar.style.width = pixelsRequired + "px";
  scaleBarText.innerHTML = targetSize ? targetSize + "cm" : "100cm";

  // Finally, set the parent div to be draggable.
  const scaleBarContainer = document.getElementById("scale_bar_div");
  dragElement(scaleBarContainer);
}
