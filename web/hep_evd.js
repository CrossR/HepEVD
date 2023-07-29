//
// HepEVD
//

import * as THREE from "three";
import * as BufferGeometryUtils from 'three/addons/utils/BufferGeometryUtils.js';
import Stats from "three/addons/libs/stats.module.js";

import { RenderState } from "./render_state.js";
import { animate, onWindowResize } from "./rendering.js";
import { saveEvd, quitEvd } from "./ui.js";

// Do some initial threejs setup...
const threeDCamera = new THREE.PerspectiveCamera(
  50,
  window.innerWidth / window.innerHeight,
  0.1,
  1e6,
);
const twoDCamera = new THREE.OrthographicCamera(
  window.innerWidth / -2,
  window.innerWidth / 2,
  window.innerHeight / 2,
  window.innerHeight / -2,
  -1,
  1e6,
);
const renderer = new THREE.WebGLRenderer({
  alpha: true,
  antialias: true,
  preserveDrawingBuffer: true,
});
const stats = new Stats();
// Move to top right.
stats.domElement.style.cssText = "position:absolute; bottom:0px; right:0px;";
renderer.setSize(window.innerWidth, window.innerHeight);

document.body.appendChild(renderer.domElement);
document.body.appendChild(stats.dom);

// Pull in the basic data from the API...
const detectorGeometry = await fetch("geometry").then((response) =>
  response.json(),
);
const hits = await fetch("hits").then((response) => response.json());
const mcHits = await fetch("mcHits").then((response) => response.json());

// And use that data to setup the initial rendering states.
const threeDRenderer = new RenderState(
  "3D",
  threeDCamera,
  renderer,
  hits.filter((hit) => hit.dim === "3D"),
  mcHits.filter((hit) => hit.dim === "3D"),
  detectorGeometry,
);
const twoDRenderer = new RenderState(
  "2D",
  twoDCamera,
  renderer,
  hits.filter((hit) => hit.dim === "2D"),
  mcHits.filter((hit) => hit.dim === "2D"),
  detectorGeometry,
);
threeDRenderer.otherRenderer = twoDRenderer;
twoDRenderer.otherRenderer = threeDRenderer;
const renderStates = new Map([
  ["3D", threeDRenderer],
  ["2D", twoDRenderer],
]);

// Prefer drawing 3D hits, but draw 2D if only option.
const defaultDraw = threeDRenderer.hitSize != 0 ? "3D" : "2D";

// For each of the 2D + 3D renderers, setup and render the geometry and hits,
// but only show the default one, as picked above.
renderStates.forEach((state) => {
  state.setupUI(defaultDraw);
});

// Hook up various global events.
document.saveEvd = () => saveEvd(renderer);
document.quitEvd = () => quitEvd();
window.addEventListener(
  "resize",
  () => {
    onWindowResize(threeDRenderer.camera, renderer);
    onWindowResize(twoDRenderer.camera, renderer);
  },
  false,
);
document.resetView = () => {
  threeDRenderer.resetView();
  twoDRenderer.resetView();
};

const markers = await fetch("markers").then((response) => response.json());
const bufferGeometry = new THREE.BufferGeometry();

const zOffset = 0.01 / markers.filter((marker) => marker.type === "V View").length;
let ringNumber = 0;
const vertices = [];
const indicies = [];

const segments = 32;
const startAngle = 0;
const endAngle = Math.PI * 2;
const theta = (endAngle - startAngle) / segments;

markers.filter((marker) => marker.type === "V View")
    .forEach((marker) => {
        if (marker.inner === 0) return;
        const innerRadius = marker.inner;
        const outerRadius = marker.outer;
        const x = marker.x;
        const z = marker.z;

        for (let i = 0; i <= segments; i++) {
            const angle = startAngle + i * theta;
            const cos = Math.cos(angle);
            const sin = Math.sin(angle);
            vertices.push(x + cos * innerRadius, z + sin * innerRadius, 0.1 + (zOffset * ringNumber));
            vertices.push(x + cos * outerRadius, z + sin * outerRadius, 0.1 + (zOffset * ringNumber));
        }

        const offset = vertices.length / 3 - (segments + 1) * 2;
        for (let i = 0; i < segments; i++) {
            indicies.push(offset + i * 2);
            indicies.push(offset + i * 2 + 1);
            indicies.push(offset + i * 2 + 2);
            indicies.push(offset + i * 2 + 2);
            indicies.push(offset + i * 2 + 1);
            indicies.push(offset + i * 2 + 3);
        }

        ringNumber += 1;
});

bufferGeometry.setAttribute("position", new THREE.Float32BufferAttribute(vertices, 3));
bufferGeometry.setIndex(indicies);

const ringMaterial = new THREE.MeshBasicMaterial( { color: 'red', transparent: true, opacity: 0.01 } );
ringMaterial.lightMapIntensisty = 0.1;
console.log(ringMaterial);

const mesh = new THREE.Mesh(bufferGeometry, ringMaterial);
mesh.matrixAutoUpdate = false;
mesh.matrixWorldAutoUpdate = false;
twoDRenderer.scene.add(mesh);

// Finally, animate the scene!
animate(renderer, renderStates, stats);
