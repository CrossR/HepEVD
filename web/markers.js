//
// Marker Drawing
//

import * as THREE from "three";

/**
 * Draws rings using the provided data and adds them to the specified group.
 * @param {Array} rings - An array of ring data objects.
 * @param {THREE.Group} group - The group to which the rings will be added.
 */
export function drawRings(rings, group) {
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
    const x = ring.x;
    const z = ring.z;

    for (let i = 0; i <= segments; i++) {
      const angle = startAngle + i * theta;
      const cos = Math.cos(angle);
      const sin = Math.sin(angle);

      const x1 = x + cos * innerRadius;
      const z1 = z + sin * innerRadius;
      const x2 = x + cos * outerRadius;
      const z2 = z + sin * outerRadius;

      [
        [x1, z1],
        [x2, z2],
      ].forEach(([x, z]) => {
        // First, store the new vertex.
        // We store the vertices with decreasing z values, so that
        // there is less chance of z-fighting.
        const index = vertices.length / 3;
        vertices.push(x, z, ringNumber * -1);
        colors.push(1, 1, 1, 0.01);

        // Now, update the map.
        // Round to nearest five, so that we can group vertices together.
        // This isn't perfect, but it's good enough for now.
        const key = `${Math.round(x / 5) * 5},${Math.round(z / 5) * 5}`;
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
