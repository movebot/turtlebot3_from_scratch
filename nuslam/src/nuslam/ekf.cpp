#include "nuslam/ekf.hpp"
#include <exception>

namespace nuslam
{
	// Used to for prediction stage
    using rigid2d::Twist2D;

    // used for model update
    using rigid2d::Pose2D;

    // used for map update
    using rigid2d::Vector2D;

	// CovarianceMatrix
	CovarianceMatrix::CovarianceMatrix()
	{
        std::vector<double> robot_state_cov{0.0, 0.0, 0.0}; // init to 0,0,0
        // 3*3
        // construct EigenVector from Vector
        Eigen::VectorXd robot_state_cov_vct = Eigen::VectorXd::Map(robot_state_cov.data(), robot_state_cov.size());
        Eigen::MatrixXd robot_cov_mtx = robot_state_cov_vct.asDiagonal();

        cov_mtx = robot_cov_mtx;
	}

	CovarianceMatrix::CovarianceMatrix(const std::vector<Point> & map_state_)
	{
        std::vector<double> robot_state_cov{0.0, 0.0, 0.0}; // init to 0,0,0
        // 3*3
        // construct EigenVector from Vector
        Eigen::VectorXd robot_state_cov_vct = Eigen::VectorXd::Map(robot_state_cov.data(), robot_state_cov.size());
        Eigen::MatrixXd robot_cov_mtx = robot_state_cov_vct.asDiagonal();

        map_state = map_state_;
        // 2n*2n diag matrix since map containts x1,y1, ... xn,yn
        // std::vector<double> map_state_cov(2 * map_state.size(), std::numeric_limits<double>::infinity()); // init to 0,0,0
        std::vector<double> map_state_cov(2 * map_state.size(), 1000); // init to 0,0,0
        // construct EigenVector from Vector
        Eigen::VectorXd map_state_cov_vct = Eigen::VectorXd::Map(map_state_cov.data(), map_state_cov.size());
        Eigen::MatrixXd map_cov_mtx = map_state_cov_vct.asDiagonal();

        // 2n*3
        Eigen::MatrixXd bottom_left = Eigen::MatrixXd::Zero(2 * map_state.size(), 3);
        // 3*2n
        Eigen::MatrixXd top_right = Eigen::MatrixXd::Zero(3, 2 * map_state.size());

        // Construct left half of matrix
        Eigen::MatrixXd left_mtx(robot_cov_mtx.rows() + bottom_left.rows(), robot_cov_mtx.cols());
        left_mtx << robot_cov_mtx, bottom_left;
        // Construct right half of matrix
        Eigen::MatrixXd right_mtx(top_right.rows() + map_cov_mtx.rows(), top_right.cols());
        right_mtx << top_right, map_cov_mtx;

        // Merge two matrixes to construct Covariance Matrix
        Eigen::MatrixXd mtx(left_mtx.rows(), left_mtx.cols() + right_mtx.cols());
        mtx << left_mtx, right_mtx;

        cov_mtx = mtx;
	}

	CovarianceMatrix::CovarianceMatrix(const std::vector<Point> & map_state_, \
                         			   const std::vector<double> & robot_state_cov_,\
			                           const std::vector<double> & map_state_cov_)
	{
        robot_state_cov = robot_state_cov_;
        // construct EigenVector from Vector
        Eigen::VectorXd robot_state_cov_vct = Eigen::VectorXd::Map(robot_state_cov.data(), robot_state_cov.size());
        Eigen::MatrixXd robot_cov_mtx = robot_state_cov_vct.asDiagonal();

        map_state = map_state_;
        map_state_cov = map_state_cov_;
        // construct EigenVector from Vector
        Eigen::VectorXd map_state_cov_vct = Eigen::VectorXd::Map(map_state_cov.data(), map_state_cov.size());
        Eigen::MatrixXd map_cov_mtx = map_state_cov_vct.asDiagonal();

        // 2n*3
        Eigen::MatrixXd bottom_left = Eigen::MatrixXd::Zero(2 * map_state.size(), 3);
        // 3*2n
        Eigen::MatrixXd top_right = Eigen::MatrixXd::Zero(3, 2 * map_state.size());

        // Construct left half of matrix
        Eigen::MatrixXd left_mtx(robot_cov_mtx.rows() + bottom_left.rows(), robot_cov_mtx.cols());
        left_mtx << robot_cov_mtx, bottom_left;
        // Construct right half of matrix
        Eigen::MatrixXd right_mtx(top_right.rows() + map_cov_mtx.rows(), top_right.cols());
        right_mtx << top_right, map_cov_mtx;

        // Merge two matrixes to construct Covariance Matrix
        Eigen::MatrixXd mtx(left_mtx.rows(), left_mtx.cols() + right_mtx.cols());
        mtx << left_mtx, right_mtx;

        cov_mtx = mtx;
	}

	// Process Noise
	ProcessNoise::ProcessNoise()
	{
		map_size = 0;

		xyt_noise = Pose2D();

		Eigen::VectorXd xyt_vct(3);

		xyt_vct << xyt_noise.theta, xyt_noise.x, xyt_noise.y;

		// assume x,y,theta noise decoupled from each other, 3*3
		Eigen::MatrixXd q = xyt_vct.asDiagonal();

		Q = q;

	}

	ProcessNoise::ProcessNoise(const Pose2D & xyt_noise_var, const unsigned long int & map_size_)
	{
		map_size = map_size_;
		// std::vector<double> noise_vect = get_3d_noise(xyt_noise_mean, xyt_noise_var, cov_mtx);
		xyt_noise = Pose2D(xyt_noise_var.x, xyt_noise_var.y, xyt_noise_var.theta);

		Eigen::VectorXd xyt_vct(3);

		xyt_vct << xyt_noise.theta, xyt_noise.x, xyt_noise.y;

		// assume x,y,theta noise decoupled from each other, 3*3
		Eigen::MatrixXd q = xyt_vct.asDiagonal();

		// 2n*3
        Eigen::MatrixXd bottom_left = Eigen::MatrixXd::Zero(2 * map_size, 3);
        // 3*2n
        Eigen::MatrixXd top_right = Eigen::MatrixXd::Zero(3, 2 * map_size);
        // 2*2n
        Eigen::MatrixXd bottom_right = Eigen::MatrixXd::Zero(2 * map_size, 2 * map_size);

        // Construct left half of matrix
        Eigen::MatrixXd left_mtx(q.rows() + bottom_left.rows(), q.cols());
        left_mtx << q, bottom_left;
        // Construct right half of matrix
        Eigen::MatrixXd right_mtx(top_right.rows() + bottom_right.rows(), top_right.cols());
        right_mtx << top_right, bottom_right;

        // Merge two matrixes to construct Covariance Matrix
        Eigen::MatrixXd mtx(left_mtx.rows(), left_mtx.cols() + right_mtx.cols());
        mtx << left_mtx, right_mtx;

        Q = mtx;

	}

	// Measurement Noise
	MeasurementNoise::MeasurementNoise()
	{
		rb_noise_var = RangeBear();

		Eigen::VectorXd rb_vct(2);

		rb_vct << rb_noise_var.range, rb_noise_var.bearing;

		R = rb_vct.asDiagonal();
	}

	MeasurementNoise::MeasurementNoise(const RangeBear & rb_noise_var_)
	{
		rb_noise_var = rb_noise_var_;

		Eigen::VectorXd rb_vct(2);

		rb_vct << rb_noise_var.range, rb_noise_var.bearing;

		R = rb_vct.asDiagonal();
	}


	// Random Sampling Functions
	std::mt19937 & get_random()
    {
        // static variables inside a function are created once and persist for the remainder of the program
        static std::random_device rd{}; 
        static std::mt19937 mt{rd()};
        // we return a reference to the pseudo-random number genrator object. This is always the
        // same object every time get_random is called
        return mt;
    }

    Eigen::VectorXd sampleNormalDistribution(int mtx_dimension)
    {
    	Eigen::VectorXd noise_vect = Eigen::VectorXd::Zero(mtx_dimension);

    	for (auto i = 0; i < mtx_dimension; i++)
    	{
    		std::normal_distribution<double> d(0, 1);
    		noise_vect(i) = d(get_random());
    	}
		
		return noise_vect;
    }

    Eigen::VectorXd getMultivarNoise(const Eigen::MatrixXd & noise_mtx)
    {
    	// Cholesky Decomposition for Multivariate Noise
    	// https://math.stackexchange.com/questions/2079137/generating-multivariate-normal-samples-why-cholesky
    	Eigen::MatrixXd L(noise_mtx.llt().matrixL());

    	int mtx_dimension = noise_mtx.cols();
    	Eigen::VectorXd noise_vect = sampleNormalDistribution(mtx_dimension);

    	return L * noise_vect;
    }

    //EKF
    EKF::EKF()
    {
    	State = Eigen::VectorXd::Zero(3);
    	max_range = 3.5;
    	robot_state = Pose2D();
    	proc_noise = ProcessNoise();
    	msr_noise = MeasurementNoise();
    	cov_mtx = CovarianceMatrix();
    	cov_mtx = cov_mtx;
    	N = 0;
    	mahalanobis_lower = 0;
		mahalanobis_upper = 0;
    }

    EKF::EKF(const Pose2D & robot_state_, const std::vector<Point> & map_state_,\
    		 const Pose2D & xyt_noise_var, const RangeBear & rb_noise_var_,\
    		 const double & max_range_, double mahalanobis_lower_, double mahalanobis_upper_)
    {
    	State = Eigen::VectorXd::Zero(3 + 2 * map_state_.size());
    	max_range = max_range_;
    	robot_state = robot_state_;
    	map_state = map_state_;
    	cov_mtx = CovarianceMatrix(map_state_);
    	cov_mtx = cov_mtx;
    	proc_noise = ProcessNoise(xyt_noise_var, map_state_.size());
    	msr_noise = MeasurementNoise(rb_noise_var_);
    	N = 0;
    	mahalanobis_lower = mahalanobis_lower_;
		mahalanobis_upper = mahalanobis_upper_;
    }

    void EKF::predict(const Twist2D & twist)
    {
    	// Angle Wrap Robot Theta
    	robot_state.theta = rigid2d::normalize_angle(robot_state.theta);
    	// First, update the estimate using the forward model
    	Eigen::VectorXd noise_vect = getMultivarNoise(proc_noise.Q);

    	Pose2D belief;

    	if (rigid2d::almost_equal(twist.w_z, 0.0))
    	// If dtheta = 0
    	{
    		belief = Pose2D(robot_state.x + (twist.v_x * cos(robot_state.theta)) + noise_vect(1),\
						    robot_state.y + (twist.v_x * sin(robot_state.theta)) + noise_vect(2),\
    						robot_state.theta + noise_vect(0));
    	} else {
		// If dtheta != 0
    		belief = Pose2D(robot_state.x + ((-twist.v_x / twist.w_z) * sin(robot_state.theta) + (twist.v_x / twist.w_z) * sin(robot_state.theta + twist.w_z)) + noise_vect(1),\
						    robot_state.y + ((twist.v_x / twist.w_z) * cos(robot_state.theta) + (-twist.v_x / twist.w_z) * cos(robot_state.theta + twist.w_z)) + noise_vect(2),\
    						robot_state.theta + twist.w_z + noise_vect(0));
    	}
    	// Angle Wrap Robot Theta
    	belief.theta = rigid2d::normalize_angle(belief.theta);

    	// Next, we propagate the uncertainty using the linearized state transition model
    	// (3+2n)*(3+2n)
    	Eigen::MatrixXd g = Eigen::MatrixXd::Zero(3 + (2 * map_state.size()), 3 + (2 * map_state.size()));
    	if (rigid2d::almost_equal(twist.w_z, 0.0))
    	// If dtheta = 0
    	{
    		// Now replace non-zero entries
    		// using theta,x,y
    		g(1, 0) = -twist.v_x * sin(robot_state.theta);
    		g(2, 0) = twist.v_x * cos(robot_state.theta);
    	} else {
		// If dtheta != 0
    		// Now replace non-zero entries
    		// using theta,x,y
    		g(1, 0) = (-twist.v_x / twist.w_z) * cos(robot_state.theta) + (twist.v_x / twist.w_z) * cos(robot_state.theta + twist.w_z);
    		g(2, 0) = (-twist.v_x / twist.w_z) * sin(robot_state.theta) + (twist.v_x / twist.w_z) * sin(robot_state.theta + twist.w_z);
    	}

    	Eigen::MatrixXd G = Eigen::MatrixXd::Identity(3 + (2 * map_state.size()), 3 + (2 * map_state.size())) + g;

    	cov_mtx.cov_mtx = G * cov_mtx.cov_mtx * G.transpose() + proc_noise.Q;

    	// store belief as new robot state for update operation
    	robot_state = belief;

    	// Update State Matrix
    	State(0) = robot_state.theta;
    	State(1) = robot_state.x;
    	State(2) = robot_state.y;
    }

    Eigen::MatrixXd EKF::inv_msr_model(const int & j)
    {
    	// x-distance to landmark
    	double x_diff = State(3 + 2*j) - State(1);
    	// y-distance to landmark
    	double y_diff = State(4 + 2*j) - State(2);
    	double squared_diff = pow(x_diff, 2) + pow(y_diff, 2);
    	// Eigen::MatrixXd h(2, 5);
    	// h << 0.0, (-x_diff / sqrt(squared_diff)), (-y_diff / sqrt(squared_diff)), (x_diff / sqrt(squared_diff)), (y_diff / sqrt(squared_diff)),
    	// 	 -1.0, (y_diff / sqrt(squared_diff)), (-x_diff / sqrt(squared_diff)), (-y_diff / sqrt(squared_diff)), (x_diff / sqrt(squared_diff));
    	// std::cout << "h: \n" << h << std::endl;
    	// 2*(2n+3)
		Eigen::MatrixXd H = Eigen::MatrixXd::Zero(2, 3 + 2 * map_state.size());
		// H constructed from four Matrices: https://nu-msr.github.io/navigation_site/slam.pdf
		// NOTE: j starts at 1 in slam.pdf
		Eigen::MatrixXd h_left(2, 3);
		h_left << 0.0, (-x_diff / sqrt(squared_diff)), (-y_diff / sqrt(squared_diff)), -1.0, (y_diff / sqrt(squared_diff)), (-x_diff / sqrt(squared_diff));

		Eigen::MatrixXd h_mid_left = Eigen::MatrixXd::Zero(2, 2*j);

		Eigen::MatrixXd h_mid_right(2, 2);
		h_mid_right << (x_diff / sqrt(squared_diff)), (y_diff / sqrt(squared_diff)), (-y_diff / sqrt(squared_diff)), (x_diff / sqrt(squared_diff));

		Eigen::MatrixXd h_right = Eigen::MatrixXd::Zero(2, 2 * map_state.size() - 2*(j + 1));

		// Construct H - Measurement Jacobian
		Eigen::MatrixXd h_mtx(h_left.rows(), h_left.cols() + h_mid_left.cols() + h_mid_right.cols() + h_right.cols());
		h_mtx << h_left, h_mid_left, h_mid_right, h_right;
		// std::cout << "h_left: \n\n" << h_left << std::endl;
		// std::cout << "h_mid_left: \n\n" << h_mid_left << std::endl;
		// std::cout << "h_mid_right: \n\n" << h_mid_right << std::endl;
		// std::cout << "h_right: \n\n" << h_right << std::endl;
		// std::cout << "size: " << map_state.size() << std::endl;
		H = h_mtx;
		return H;
    }

    void EKF::msr_update(const std::vector<Point> & measurements_)
    {
    	// By incorporating one measurement at a time, we improve our state estimate over time
    	// and are thus improving the accuracy of our linearization and getting better EKFSLAM performance
    	for (auto iter = measurements_.begin(); iter != measurements_.end(); iter++)
    	{
    		// std::cout << "----------------------------------" << std::endl;
    		// Current Landmark Index
    		// auto j = std::distance(measurements_.begin(), iter);

    		// Mahalanobis Distance Test

    		//  Add noise to actual range, bearing measurement
	    	Eigen::VectorXd noise_vect = getMultivarNoise(msr_noise.R);
    		Eigen::VectorXd z(2);
    		z << iter->range_bear.range + noise_vect(0), iter->range_bear.bearing + noise_vect(1);
    		z(1) = rigid2d::normalize_angle(z(1));
    		// std::cout << "z: " << z << std::endl;

    		std::vector<double> d_k = mahalanobis_test(z);

    		// Find minimum mahalanobis distance d* index of d*
			auto d_star_index = std::min_element(d_k.begin(), d_k.end()) - d_k.begin();
			double d_star = d_k.at(d_star_index);

			// std::cout << "dstar " << d_star << std::endl;

			if (d_star < mahalanobis_lower or d_star > mahalanobis_upper )
			{
				int i = 0;
				if (d_star < mahalanobis_lower)
				{
					i = d_star_index;
					// std::cout << "Landmark i #: " << i << std::endl;

				} else if (d_star > mahalanobis_upper)
				{
					// Increment N
					i = N;
					N += 1;

					// Add New Landmark to State
					if (N <= map_state.size())
					{
						State(3 + 2*i) = iter->pose.x + State(1);
						State(4 + 2*i) = iter->pose.y + State(2);
					}

					// std::cout << "New Landmark index #: " << i << std::endl;

				}

				if (N <= map_state.size())
				{
					// std::cout << "N: " << N << std::endl;
					// std::cout << "dstar index " << d_star_index << std::endl;
					// std::cout << "dstar " << d_star << std::endl;

					// std::cout << "Measurement # " << j << std::endl;

					// std::cout << "i: " << i << std::endl;

					// Add measurement to state and incorporate into EKF
					// First, get theoretical expected measurement based on belief in [r,b] format
			    	// Pose difference between robot and landmark
			    	Vector2D cartesian_measurement = Vector2D(State(3 + 2*i) - State(1), State(4 + 2*i) - State(2));
			    	RangeBear polar_measurement = cartesianToPolar(cartesian_measurement);
			    	// Angle Wrap Bearing
			    	polar_measurement.bearing = rigid2d::normalize_angle(polar_measurement.bearing);
			    	// Subtract robot heading from bearing
			    	polar_measurement.bearing -= State(0);
			    	// Angle Wrap Bearing
			    	polar_measurement.bearing = rigid2d::normalize_angle(polar_measurement.bearing);

			    	Eigen::VectorXd z_hat(2);
		    		z_hat << polar_measurement.range, polar_measurement.bearing;
		    		// std::cout << "z_hat: \n" << z_hat << std::endl;

			    	// Compute the measurement Jacobian
			    	Eigen::MatrixXd H = inv_msr_model(i);
			    	// std::cout << "H: \n\n" << H << std::endl;

			    	// Compute the Kalman gain from the linearized measurement model
			    	// (2n+3)*2
					Eigen::MatrixXd K = cov_mtx.cov_mtx * H.transpose() * (H * cov_mtx.cov_mtx * H.transpose() + msr_noise.R).inverse();
					// std::cout << "K: \n" << K << std::endl;

			    	// Compute the posterior state update
		    		Eigen::VectorXd z_diff(2);
		    		z_diff = z - z_hat;
		    		// Angle Wrap Bearing
			    	z_diff(1) = rigid2d::normalize_angle(z_diff(1));
		    		// std::cout << "z_diff: \n" << z_diff << std::endl;

		    		// (2n+3)*1
		    		Eigen::VectorXd K_update = K * z_diff;
		    		// std::cout << "K_update: \n" << K_update << std::endl;
			    	State += K_update;
			    	State(0) = rigid2d::normalize_angle(State(0));
			 
			    	// Compute the posterior covariance
			    	cov_mtx.cov_mtx = (Eigen::MatrixXd::Identity(3 + (2 * map_state.size()), 3 + (2 * map_state.size())) - K * H) * cov_mtx.cov_mtx;
			    	// std::cout << "cov_mtx.cov_mtx: \n" << cov_mtx.cov_mtx << std::endl;
				} else {
					// throw std::invalid_argument("N CANNOT EXCEED MAXIMUM NUMBER OF LANDMARKS");
					N = map_state.size();
				}
			}
    		
    	}

    	// Update returnable values (robot and map state)
    	robot_state.theta = State(0);
		robot_state.theta = rigid2d::normalize_angle(robot_state.theta);
		robot_state.x = State(1);
		robot_state.y = State(2);
		// Map data starts at index 3
		// Perform update for full map state
		for (long unsigned int i = 0; i < map_state.size(); i++)
		{
		map_state.at(i).pose.x = State(3 + 2*i);
		map_state.at(i).pose.y = State(4 + 2*i);
    	}
    }

    std::vector<double> EKF::mahalanobis_test(const Eigen::VectorXd & z)
    {
    	// steps 10-18 in Probabilistic Robotics, EKFSLAM with Unknown Data Association
    	// for k = 0 to k < N (N from 0 to max_num_landmarks) | if N=0, skip

    	// First, create a vector of size N to store our mahalanobis distances
    	// if N = 0, skip loop and return empty vector, which will indicate that the
    	// current measurement is our first landmark, and it will automatically be initialized
    	std::vector<double> d_k;

    	// Populate since loop will be skipped if N=0
 		if (N == 0)
 		{
 			d_k.push_back(1e12);
 		}

    	// Step 10 PR
    	for (unsigned int k = 0; k < N; k++)
    	{
    		// Steps 11-15
    		Eigen::MatrixXd H = inv_msr_model(k);
    		// Step 16
    		Eigen::MatrixXd psi = H * cov_mtx.cov_mtx * H.transpose() + msr_noise.R;

    		// Step 17
    		Vector2D cartesian_measurement = Vector2D(State(3 + 2*k) - State(1), State(4 + 2*k) - State(2));
	    	RangeBear polar_measurement = cartesianToPolar(cartesian_measurement);
	    	// Angle Wrap Bearing
	    	polar_measurement.bearing = rigid2d::normalize_angle(polar_measurement.bearing);
	    	// Subtract robot heading from bearing
	    	polar_measurement.bearing -= State(0);
	    	// Angle Wrap Bearing
	    	polar_measurement.bearing = rigid2d::normalize_angle(polar_measurement.bearing);

	    	Eigen::VectorXd z_hat(2);
    		z_hat << polar_measurement.range, polar_measurement.bearing;

			Eigen::VectorXd z_diff(2);
    		z_diff = z - z_hat;
    		// Angle Wrap Bearing
	    	z_diff(1) = rigid2d::normalize_angle(z_diff(1));    		

    		double d = z_diff.transpose() * psi.inverse() * z_diff;

	    	d_k.push_back(d);
    	}
    	// step 18
    	return d_k;
    }

    // Eigen::MatrixXd EKF::nearestSPD(const Eigen::MatrixXd & mtx)
    // {

    // }

    Pose2D EKF::return_pose()
    {
    	return robot_state;
    }

    std::vector<Point> EKF::return_map()
    {
    	return map_state;
    }

    void EKF::reset_pose(const Pose2D & pose)
    {
    	robot_state = pose;
    }
}