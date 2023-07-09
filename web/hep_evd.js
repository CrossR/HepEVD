import * as THREE from "three";
import { OrbitControls } from "three/addons/controls/OrbitControls.js";
import { Lut } from "three/addons/math/Lut.js";

import Stats from "three/addons/libs/stats.module.js";

// ============================================================================
// Helper functions
// ============================================================================

// Convert a JS position object to threejs.
function positionToVector(position) {
  return new THREE.Vector3(position.x, position.y, position.z);
}

// Given a geometry object, fit the camera to the center of it, such that the
// whole object can be seen.
function fitSceneInCamera(camera, controls, detectorGeometry) {
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

// Build up a map between a property name and a property.
//
// This should mean that any property can be easily drawn without
// needing to worry about the specific location of a property.
function getHitProperties(hits) {
  const hitPropMap = new Map();

  // Every hit should have an energy property, but there is
  // then two additional cases where a hit can be grouped:
  //  - If a hit is labelled.
  //  - If a hit has a property map associated.
  hits.forEach((hit) => {
    hitPropMap.set(hit, new Map([["energy", hit.energy]]));

    if (Object.hasOwn(hit, "label")) {
      hitPropMap.get(hit).set(hit.label, 1.0);
    }

    if (Object.hasOwn(hit, "properties")) {
      hit.properties.forEach((prop) => {
        const key = Object.keys(prop)[0];
        const value = Object.values(prop)[0];

        hitPropMap.get(hit).set(key, value);
      });
    }
  });

  return hitPropMap;
}

// ============================================================================
// Rendering Functions
// ============================================================================

// Given a box based detector volume, draw its wireframe.
function drawBoxVolume(group, material, box) {
  const boxGeometry = new THREE.BoxGeometry(box.xWidth, box.yWidth, box.zWidth);
  const boxEdges = new THREE.EdgesGeometry(boxGeometry);
  const boxLines = new THREE.LineSegments(boxEdges, material);

  boxLines.position.set(box.x, box.y, box.z);
  boxLines.updateMatrixWorld();

  group.add(boxLines);
}

// Draw an array of 3D hits to the screen, utilising an InstancedMesh for
// performance.
//
// Optionally, colour the hits based on some property value.
function drawThreeDHits(
  group,
  material,
  hits,
  hitPropMap,
  useColour = false,
  colourProp = "",
  hitSize = 3
) {
  const hitGeometry = new THREE.BoxGeometry(hitSize, hitSize, hitSize);
  const hitMesh = new THREE.InstancedMesh(hitGeometry, material, hits.length);

  const dummyObject = new THREE.Object3D();
  const energyLut = new Lut("rainbow", 512);

  const properties = new Map();
  hits.forEach((hit, index) => {
    if (!hitPropMap.has(hit)) {
      return;
    }

    if (!hitPropMap.get(hit).has(colourProp)) {
      return;
    }

    properties.set(index, hitPropMap.get(hit).get(colourProp));
  });

  const usingColour =
    useColour &&
    properties.size > 0 &&
    [...properties.values()][0].constructor == Number;

  if (usingColour) {
    let minColourValue = Infinity;
    let maxColourValue = Number.NEGATIVE_INFINITY;
    properties.forEach((value, _) => {
      if (value < minColourValue) minColourValue = value;
      if (value > maxColourValue) maxColourValue = value;
    });
    energyLut.setMax(maxColourValue);
  }

  hits.forEach(function (hit, index) {
    dummyObject.position.set(hit.x, hit.y, hit.z);
    dummyObject.updateMatrix();

    if (usingColour && ! properties.has(index)) {
      return;
    }

    hitMesh.setMatrixAt(index, dummyObject.matrix);

    if (usingColour) {
      console.log(properties.get(index));
      hitMesh.setColorAt(index, energyLut.getColor(properties.get(index)));
    } else {
      hitMesh.setColorAt(index, new THREE.Color("gray"));
    }
  });

  hitMesh.instanceMatrix.needsUpdate = true;
  hitMesh.instanceColor.needsUpdate = true;

  group.add(hitMesh);
}

function animate() {
  requestAnimationFrame(animate);
  renderer.render(scene, camera);
  stats.update();
}

// ============================================================================
// GUI Functions
// ============================================================================

function threeDHitsToggle(hits, threeDHitGroupMap, hitPropMap, toggleTarget) {
  if (toggleTarget === "none") {
    threeDHitGroupMap.forEach((group) => (group.visible = false));
    return;
  }

  // If it does exist, then just toggle its visibility.
  if (threeDHitGroupMap.has(toggleTarget) === true) {
    const threeDHitGroup = threeDHitGroupMap.get(toggleTarget);
    threeDHitGroup.visible = !threeDHitGroup.visible;

    return;
  }

  // Otherwise, we need to make a new group, populate it and store it for later.
  const newGroup = new THREE.Group();
  scene.add(newGroup);
  drawThreeDHits(newGroup, materialHit, hits, hitPropMap, true, toggleTarget);
  threeDHitGroupMap.set(toggleTarget, newGroup);

  return;
}

// Given a drop down,
function populateDropdown(className, dropdownID, entries, onClick = (_) => {}) {
  const dropDown = document.getElementById(dropdownID);

  entries.forEach((entry) => {
    const newButton = document.createElement("button");
    newButton.innerText = entry;
    newButton.id = `${className}_${entry}`
    newButton.addEventListener("click", () => onClick(entry));
    dropDown.appendChild(newButton);
  });

  return;
}

// Toggle active state of a given button
function toggleButton(className, ID) {
  const button = document.getElementById(`${className}_${ID}`);

  const isActive = button.style.color === "white";

  if (isActive) {
    button.style.color = "green";
  } else {
    button.style.color = "white";
  }
}

// ============================================================================
// HepEVD
// ============================================================================

// First, do the initial threejs setup.
// That is the scene/camera/renderer/controls, and some basic properties of each.
// Once complete, add to the actual web page.
const scene = new THREE.Scene();
const camera = new THREE.PerspectiveCamera(
  50,
  window.innerWidth / window.innerHeight,
  0.1,
  1e6
);
const renderer = new THREE.WebGLRenderer();
const controls = new OrbitControls(camera, renderer.domElement);
const stats = new Stats();
renderer.setSize(window.innerWidth, window.innerHeight);

if (document.body.className === "lighttheme") renderer.setClearColor("white");
else renderer.setClearColor("black");

renderer.alpha = true;
renderer.antialias = false;
document.body.appendChild(renderer.domElement);
document.body.appendChild(stats.dom);

// Then, setup some basic materials and data structures the rest of the renderer can use.
const materialLine = new THREE.LineBasicMaterial({ color: "gray" });
const materialGeometry = new THREE.LineBasicMaterial({ color: "darkred" });
const materialHit = new THREE.MeshBasicMaterial({
  side: THREE.DoubleSide,
});

const detectorGeometryGroup = new THREE.Group();
scene.add(detectorGeometryGroup);

// 3D hits are stored in multiple groups, such that they can be toggled independantly.
const threeDHitGroupMap = new Map();
const threeDHitGroup = new THREE.Group();
scene.add(threeDHitGroup);
threeDHitGroupMap.set("default", threeDHitGroup);

// Finally, start pulling in data about the event.
const detectorGeometry = await fetch("geometry").then((response) =>
  response.json()
);
const hits = await fetch("hits").then((response) => response.json());
const hitPropMap = getHitProperties(hits);

// Time to start the actual rendering.
detectorGeometry
  .filter((volume) => volume.type === "box")
  .forEach((box) =>
    drawBoxVolume(detectorGeometryGroup, materialGeometry, box)
  );
drawThreeDHits(
  threeDHitGroup,
  materialHit,
  hits.filter((hit) => hit.type === "3D"),
  hitPropMap
);

// Populate the UI properly.
// This includes functions that the GUI uses, and filling in the various dropdowns.
let threeDHitsDropDownOnClick = (toggleTarget) => {
  threeDHitsToggle(hits, threeDHitGroupMap, hitPropMap, toggleTarget);
  toggleButton("threeD", toggleTarget);
};
const threeDHitProperties = new Set();
hitPropMap.forEach((properties, _) => {
  properties.forEach((_, propString) => threeDHitProperties.add(propString));
});
document.threeDHitsToggle = threeDHitsDropDownOnClick;
populateDropdown(
  "threeD",
  "threeD_dropdown",
  threeDHitProperties,
  threeDHitsDropDownOnClick
);
toggleButton("threeD", "default");

// Start the final rendering of the event.

// Irient the camera to the middle of the scene.
fitSceneInCamera(camera, controls, detectorGeometryGroup);

// Finally, animate the scene!
animate();

// Once run once, we can disable these to help with performance.
// Noting is animated in our scene, so not needed.
scene.matrixAutoUpdate = false;
scene.autoUpdate = false;
