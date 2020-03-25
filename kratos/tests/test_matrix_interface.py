﻿from __future__ import print_function, absolute_import, division

import KratosMultiphysics.KratosUnittest as KratosUnittest
import KratosMultiphysics as KM
import math

class TestMatrixInterface(KratosUnittest.TestCase):

    def test_copy(self):
        A = KM.Matrix(2,3)
        A.fill(1.0)

        b = KM.Vector(3)
        b[0] = 1.0
        b[1] = 2.0
        b[2] = 3.0

        A2 = KM.Matrix(A)
        b2 = KM.Vector(b)

        self.assertEqual(2,A2.Size1())
        self.assertEqual(3,A2.Size2())

        for j in range(A2.Size2()):
            self.assertEqual(b2[j], b2[j])
            for i in range(A2.Size1()):
                self.assertEqual(A[i,j], A2[i,j])

        A[0,1] = 2.0
        self.assertEqual(1.0, A2[0,1])
        b[0] = 2.0
        self.assertEqual(1.0, b2[0])

    def test_assignement(self):
        A = KM.Matrix(2,3)

        self.assertEqual(2,A.Size1())
        self.assertEqual(3,A.Size2())

        for i in range(A.Size1()):
            for j in range(A.Size2()):
                A[i,j] = i+j

        for j in range(A.Size2()):
            for i in range(A.Size1()):
                self.assertEqual(A[i,j], i+j)

    def test_matrix_vector(self):
        A = KM.Matrix(4,3)

        for i in range(A.Size1()):
            for j in range(A.Size2()):
                A[i,j] = i

        #matrix vector
        b = KM.Vector(3)
        b.fill(1.0)
        c = A*b
        for i in range(len(c)):
            self.assertEqual(c[i],i*A.Size2())

        #matrix array_1d<double,3>
        b = KM.Array3(1.0)
        c = A*b
        for i in range(len(c)):
            self.assertEqual(c[i],i*A.Size2())


    def test_matrix_sum(self):
        A = KM.Matrix(2,3)
        A.fill(1.0)
        B = KM.Matrix(2,3)
        B.fill(2.0)
        C = A+B

        for i in range(A.Size1()):
            for j in range(A.Size2()):
                self.assertEqual(C[i,j], 3.0)

        A += B

        for i in range(A.Size1()):
            for j in range(A.Size2()):
                self.assertEqual(C[i,j], A[i,j])

    def test_matrix_diff(self):
        A = KM.Matrix(2,3)
        A.fill(1.0)
        B = KM.Matrix(2,3)
        B.fill(2.0)
        C = A-B

        for i in range(A.Size1()):
            for j in range(A.Size2()):
                self.assertEqual(C[i,j], -1.0)

        A -= B
        for i in range(A.Size1()):
            for j in range(A.Size2()):
                self.assertEqual(C[i,j], A[i,j])

    def test_scalar_prod(self):
        A = KM.Matrix(2,3)
        A.fill(2.0)
        C = A*2.0

        for i in range(A.Size1()):
            for j in range(A.Size2()):
                self.assertEqual(C[i,j], 4.0)

        A *= 2.0
        for i in range(A.Size1()):
            for j in range(A.Size2()):
                self.assertEqual(C[i,j], A[i,j])

if __name__ == '__main__':
    KratosUnittest.main()
