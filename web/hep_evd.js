import * as THREE from "three";
import { OrbitControls } from "three/addons/controls/OrbitControls.js";
import Stats from "three/addons/libs/stats.module.js";

// ============================================================================
// Helper functions
// ============================================================================

function positionToVector(position) {
  return new THREE.Vector3(position.x, position.y, position.z);
}

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

// ============================================================================
// Rendering Functions
// ============================================================================

function drawBoxVolume(group, material, box) {
  const boxGeometry = new THREE.BoxGeometry(box.xWidth, box.yWidth, box.zWidth);
  const boxEdges = new THREE.EdgesGeometry(boxGeometry);
  const boxLines = new THREE.LineSegments(boxEdges, material);

  boxLines.position.set(box.x, box.y, box.z);
  boxLines.updateMatrixWorld();

  group.add(boxLines);
}

function drawThreeDHits(group, material, hit, hit_size = 1) {
  const hitGeometry = new THREE.BoxGeometry(hit_size, hit_size, hit_size);
  const hitEdges = new THREE.EdgesGeometry(hitGeometry);
  const hitMesh = new THREE.InstancedMesh(hitEdges, material, hits.length);

  const dummyObject = new THREE.Object3D();

  hits.forEach(function (hit, index) {
    dummyObject.position.set(hit.x, hit.y, hit.z);
    dummyObject.updateMatrix();

    hitMesh.setMatrixAt(index, dummyObject.matrix);
  });

  group.add(hitMesh);
}

function animate() {
  requestAnimationFrame(animate);
  renderer.render(scene, camera);
  stats.update();
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
  color: "gray",
  side: THREE.DoubleSide,
});

const detectorGeometryGroup = new THREE.Group();
scene.add(detectorGeometryGroup);
const threeDHitGroup = new THREE.Group();
scene.add(threeDHitGroup);

// Finally, start pulling in data about the event.
const detectorGeometry = await fetch("geometry").then((response) =>
  response.json()
);
const hits = await fetch("hits").then((response) => response.json());

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

// Start the final rendering of the event.

// Irient the camera to the middle of the scene.
fitSceneInCamera(camera, controls, detectorGeometryGroup);

// Finally, animate the scene!
animate();

// Once run once, we can disable these to help with performance.
// Noting is animated in our scene, so not needed.
scene.matrixAutoUpdate = false;
scene.autoUpdate = false;
