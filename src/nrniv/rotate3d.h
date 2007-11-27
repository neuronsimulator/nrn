#ifndef rotation3d_h
#define rotation3d_h

/*  3-D rotation matrix */

class Rotation3d : public Resource {
public:
	Rotation3d();
	virtual ~Rotation3d();
	
	void rotate(float x, float y, float z, float* tr)const;
	void rotate(float* r, float* tr) const;
	void inverse_rotate(float* tr, float* r) const;

	void identity();
	
	// relative transformation of the transformation
	void rotate_x(float radians);
	void rotate_y(float radians);
	void rotate_z(float radians);
	void origin(float x, float y, float z);
	void offset(float x, float y);
	void post_multiply(Rotation3d&);
	void x_axis(float& x, float& y) const;
	void y_axis(float& x, float& y) const;
	void z_axis(float& x, float& y) const;
private:
	float a_[3][3];
	float o_[2][3];
};
#endif
