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

// Build up a <string, array[hits]> map, linking every available hit property.
function getHitProperties(hits) {
  const hitPropMap = new Map();

  // Create a default entry for energy and time,
  // since every hit should have this info.
  hitPropMap.set("energy", hits);
  hitPropMap.set("time", hits);

  const setOrAdd = (hit, prop) => {
    if (hitPropMap.has(prop)) {
      hitPropMap.get(prop).push(hit);
    } else {
      hitPropMap.set(prop, [hit]);
    }
  };

  hits.forEach((hit) => {
    if (Object.hasOwn(hit, "label")) {
      setOrAdd(hit, hit.label);
    }

    if (Object.hasOwn(hit, "properties")) {
      hit.properties.forEach((prop) => setOrAdd(hit, prop));
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
  useColour = false,
  colourProp = "",
  hitSize = 3
) {
  const hitGeometry = new THREE.BoxGeometry(hitSize, hitSize, hitSize);
  const hitMesh = new THREE.InstancedMesh(hitGeometry, material, hits.length);

  const dummyObject = new THREE.Object3D();
  const energyLut = new Lut("rainbow", 512);

  if (useColour) {
    energyLut.setMax(
      hits.reduce(
        (max, hit) => (hit[colourProp] > max ? hit[colourProp] : max),
        0
      )
    );
  }

  hits.forEach(function (hit, index) {
    dummyObject.position.set(hit.x, hit.y, hit.z);
    dummyObject.updateMatrix();

    hitMesh.setMatrixAt(index, dummyObject.matrix);
    if (useColour) {
      console.log(hit[colourProp])
      hitMesh.setColorAt(index, energyLut.getColor(hit[colourProp]));
    } else {
      hitMesh.setColorAt(index, new THREE.Color('gray'));
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

function threeDHitsToggle(
  threeDHitGroupMap,
  hitPropertiesMap,
  toggleTarget
) {

  if (toggleTarget === "none") {
    threeDHitGroupMap.forEach((group) => (group.visible = false));
    return;
  }

  const hasGroup = threeDHitGroupMap.has(toggleTarget) === true;
  const targetExists = hitPropertiesMap.has(toggleTarget) === true;

  // If the property doesn't exist, we can't do anything.
  if (! hasGroup && ! targetExists) {
    return;
  }

  // If it does exist, then just toggle its visibility.
  if (hasGroup) {
    const threeDHitGroup = threeDHitGroupMap.get(toggleTarget);
    threeDHitGroup.visible = !threeDHitGroup.visible;

    return;
  }

  // If we are here, the property exists, but we haven't actually rendered it
  // yet.  So make a new group, populate it, store it for later.
  const newGroup = new THREE.Group();
  scene.add(newGroup);
  drawThreeDHits(newGroup, materialHit, hitPropertiesMap.get(toggleTarget), true, toggleTarget);
  threeDHitGroupMap.set(toggleTarget, newGroup);

  return;
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
const hitPropertiesMap = getHitProperties(hits);

// Time to start the actual rendering.
detectorGeometry
  .filter((volume) => volume.type === "box")
  .forEach((box) =>
    drawBoxVolume(detectorGeometryGroup, materialGeometry, box)
  );
drawThreeDHits(
  threeDHitGroup,
  materialHit,
  hits.filter((hit) => hit.type === "3D")
);

// Populate the UI properly.
// This includes functions that the GUI uses, and filling in the various dropdowns.
document.threeDHitsToggle = (toggleTarget) =>
  threeDHitsToggle(threeDHitGroupMap, hitPropertiesMap, toggleTarget);

// Start the final rendering of the event.

// Irient the camera to the middle of the scene.
fitSceneInCamera(camera, controls, detectorGeometryGroup);

// Finally, animate the scene!
animate();

// Once run once, we can disable these to help with performance.
// Noting is animated in our scene, so not needed.
scene.matrixAutoUpdate = false;
scene.autoUpdate = false;
