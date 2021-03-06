#include "Controller_invariant_Pid_SRIvar_Tracking.h"
// #include <Eigen/Dense>
// #include <iostream>

using namespace Eigen;


namespace DQ_robotics
{

Controller_invariant_Pid_SRIvar_Tracking::Controller_invariant_Pid_SRIvar_Tracking(DQ_kinematics robot, VectorXd upper_joint_limits, VectorXd lower_joint_limits, MatrixXd feedback_gain, double beta, double lambda_max, double epsilon) : DQ_controller()
{
    
    //Constants
    dq_one_ = DQ(1);

    //Initialization of argument parameters
    robot_dofs_     = (robot.links() - robot.n_dummy());
    robot_          = robot;
    kp_             = feedback_gain;
    ki_             = MatrixXd::Zero(feedback_gain.rows(),feedback_gain.cols());
    kd_             = MatrixXd::Zero(feedback_gain.rows(),feedback_gain.cols());    
    beta_           = beta;
    lambda_max_     = lambda_max;

    //Initilization of remaining parameters
    thetas_         = MatrixXd(robot_dofs_,1);
    delta_thetas_   = MatrixXd::Zero(robot_dofs_,1);

    task_jacobian_                 = MatrixXd(8,robot_dofs_);
    task_jacobian_pseudoinverse_   = MatrixXd(robot_dofs_,8);
    svd_                           = JacobiSVD<MatrixXd>(robot_dofs_,8);
    svd_sigma_inverted_            = MatrixXd::Zero(robot_dofs_,8);
    identity_                      = Matrix<double,8,8>::Identity();

    error_             = MatrixXd(8,1);
    integral_error_     = MatrixXd::Zero(8,1);
    last_error_         = MatrixXd::Zero(8,1);
    at_least_one_error_ = false;    

    // New terms included in proper error [NEW ERROR]
    enable_trackintTerm = true;
    enable_trackintTerm = false;
    trackingTerm_           = MatrixXd::Zero(8,1);    
    last_reference_         = DQ(0,0,0,0,0,0,0,0);
    at_least_one_reference_ = false;


    end_effector_pose_ = DQ(0,0,0,0,0,0,0,0);
    
    upper_joint_limits_    = upper_joint_limits;
    lower_joint_limits_    = lower_joint_limits;
    original_dummy_joints_ = robot_.dummy(); 
}



Controller_invariant_Pid_SRIvar_Tracking::Controller_invariant_Pid_SRIvar_Tracking( const DQ_kinematics& robot, VectorXd upper_joint_limits, VectorXd lower_joint_limits, const MatrixXd& feedback_gain, const MatrixXd& ki, const double ki_memory,  const MatrixXd& kd, const double& beta, const double& lambda_max, const double& epsilon) : DQ_controller()
{

   //Constants
    dq_one_ = DQ(1);

    //Initialization of argument parameters
    robot_dofs_     = (robot.links() - robot.n_dummy());
    robot_          = robot;
    kp_             = feedback_gain;
    ki_             = ki;
    kd_             = kd;
    ki_memory_      = ki_memory;    
    beta_           = beta;
    lambda_max_     = lambda_max;

    //Initilization of remaining parameters
    thetas_         = MatrixXd(robot_dofs_,1);
    delta_thetas_   = MatrixXd::Zero(robot_dofs_,1);

    task_jacobian_                 = MatrixXd(8,robot_dofs_);
    task_jacobian_pseudoinverse_   = MatrixXd(robot_dofs_,8);
    svd_                           = JacobiSVD<MatrixXd>(robot_dofs_,8);
    svd_sigma_inverted_            = MatrixXd::Zero(robot_dofs_,8);
    identity_                      = Matrix<double,8,8>::Identity();

    error_             = MatrixXd(8,1);
    integral_error_     = MatrixXd::Zero(8,1);
    last_error_         = MatrixXd::Zero(8,1);
    at_least_one_error_ = false;    

    // New terms included in proper error and tracking  [NEW ERROR]
    enable_trackintTerm = true;
    enable_trackintTerm = false;
    
    trackingTerm_           = MatrixXd::Zero(8,1);     
    last_reference_         = DQ(0,0,0,0,0,0,0,0);
    at_least_one_reference_ = false;

    end_effector_pose_ = DQ(0,0,0,0,0,0,0,0);
    
    upper_joint_limits_    = upper_joint_limits;
    lower_joint_limits_    = lower_joint_limits;
    original_dummy_joints_ = robot_.dummy(); 
}


VectorXd Controller_invariant_Pid_SRIvar_Tracking::getNewJointPositions( const DQ reference, const VectorXd thetas)
{

    delta_thetas_ = getNewJointVelocities(reference, thetas);

    // Send updated thetas to simulation
    return (thetas_ + delta_thetas_);

}

VectorXd Controller_invariant_Pid_SRIvar_Tracking::getNewJointVelocities( const DQ reference, const VectorXd thetas)
{

    thetas_ = thetas; //This is necessary for the getNewJointPositions to work

    DQ robot_base = robot_.base();
    DQ robot_effector = robot_.effector();

    VectorXd pseudo_dummy_joint_marker = VectorXd::Zero(robot_dofs_);
    VectorXd pseudo_thetas = thetas_;
    VectorXd pseudo_delta_thetas = VectorXd::Zero(robot_dofs_);
    VectorXd possible_new_thetas = VectorXd::Zero(robot_dofs_);    

    MatrixXd original_dh_matrix = robot_.getDHMatrix();
    MatrixXd step_dh_matrix = original_dh_matrix;
    DQ_kinematics pseudo_robot(original_dh_matrix);
    pseudo_robot.set_base(robot_base);
    pseudo_robot.set_effector(robot_effector);

    bool should_break_loop = false;
    while(not should_break_loop){

        //Calculate jacobian (N = Hminus8(x_d) * C8 * J  ) [NEW ERROR]
        task_jacobian_  = pseudo_robot.analyticalJacobian(pseudo_thetas);
        task_jacobian_  = Hminus8(reference)*C8()*task_jacobian_;
     
  
        // Recalculation of measured data.
        end_effector_pose_ = pseudo_robot.fkm(pseudo_thetas);

        //Error: [NEW ERROR] invariant to transformations
        last_error_      = error_;
        error_           = vec8( dq_one_ - (end_effector_pose_.conj())*reference );
        // error_           = vec8(reference - end_effector_pose_);
        integral_error_ = error_ + ki_memory_*integral_error_;
        //error_ = vec8(dq_one_ - conj(end_effector_pose_)*reference);


        ///Inverse calculation
        svd_.compute(task_jacobian_, ComputeFullU);
        singular_values_ = svd_.singularValues();        

        //Damping Calculation
        int current_step_relevant_dof = ( pseudo_robot.links() - pseudo_robot.n_dummy() );
        double sigma_min = singular_values_( current_step_relevant_dof - 1);
        VectorXd u_min = svd_.matrixU().col( current_step_relevant_dof - 1);
        double lambda = lambda_max_;
        if (sigma_min < epsilon_)
        {
            lambda = (1-(sigma_min/epsilon_)*(sigma_min/epsilon_))*lambda_max_*lambda_max_;
        }

        task_jacobian_pseudoinverse_ =    (task_jacobian_.transpose())*((task_jacobian_*task_jacobian_.transpose() 
                                            + (beta_*beta_)*identity_ + (lambda*lambda)*u_min*u_min.transpose()).inverse());


        // Trackink term [NEW ERROR]
        if( at_least_one_reference_ )
          trackingTerm_ = Hminus8( reference - last_reference_ )*C8()*vec8( end_effector_pose_ );
        else
          at_least_one_reference_ = true;        
        last_reference_ = reference;
        // If trackint term is disabled
        if (enable_trackintTerm == false)
            trackingTerm_  = MatrixXd::Zero(8,1);    


    

        // pseudo_delta_thetas = task_jacobian_pseudoinverse_*kp_*error_;  [NEW ERROR]
        if( at_least_one_error_ )
          pseudo_delta_thetas = task_jacobian_pseudoinverse_*( kp_*error_ + ki_*integral_error_ + kd_*(error_ - last_error_)  - trackingTerm_ );
        else
        {
          at_least_one_error_ = true;
          pseudo_delta_thetas = task_jacobian_pseudoinverse_*( kp_*error_ + ki_*integral_error_  - trackingTerm_ );
        }


        //Update delta_thetas for possiblenewthetas calculation
        for(int i=0,j=0; i < robot_dofs_; i++)
        {
            if(pseudo_dummy_joint_marker(i) == 0)
            {
                delta_thetas_(i) = pseudo_delta_thetas(j);
                ++j;
            }
            //Do NOTHING if it should be ignored.
        }



        //Possible new thetas
        possible_new_thetas = thetas_ + delta_thetas_;

        //Verify if loop should end
        should_break_loop = true;
        int j=0;
        //For all joints
        for(int i = 0; i < robot_dofs_; i++){

            //If joint is not yet marked for not being considered in the minimization
            if(pseudo_dummy_joint_marker(i) == 0){

                //If the controller is trying to put a joint further than any of its limits
                if(    possible_new_thetas(i) > upper_joint_limits_(i)
                    || possible_new_thetas(i) < lower_joint_limits_(i) )
                {

                    //If the joint was already saturated sometime ago
                    double ep = 1.e-05;
                    if (    thetas_(i) > upper_joint_limits_(i) - ep 
                         || thetas_(i) < lower_joint_limits_(i) + ep){

                        pseudo_dummy_joint_marker(i) = 1; //Mark it to be ignored in the minization
                        //std::cout << std::endl << "Joint " << i << " will be ignored in the next controller step.";
                        step_dh_matrix(4,i) = 1;          //Set matrix as dummy.
                        step_dh_matrix(0,i) = thetas_(i); //Set matrix theta as a fixed value.
                        should_break_loop = false;
                    }
                    //If the joint was not yet saturated and the controller wants to saturate it
                    else{

                        // Saturate the joint in this step.
                        if   ( possible_new_thetas(i) > upper_joint_limits_(i) ){
                            delta_thetas_(i) = upper_joint_limits_(i) - thetas_(i);
                            //std::cout << std::endl << "Joint = " << i << " was saturated in its upper_limit";
                        }
                        else if ( possible_new_thetas(i) < lower_joint_limits_(i) ){
                            delta_thetas_(i) = lower_joint_limits_(i) - thetas_(i);
                            //std::cout << std::endl << "Joint = " << i << " was saturated in its lower_limit";
                        }
                        else{
                            std::cout << std::endl << "Something is really wrong";
                        }
                        //The joint should still be considered in the minimizations.
                        pseudo_thetas(j) = thetas_(i);
                        ++j;    
                    }

      
                }
                //If the controller is not trying to put this joint further than any of its limits, we consider the velocity given normally
                else{
                    delta_thetas_(i) = pseudo_delta_thetas(j);
                    pseudo_thetas(j) = thetas_(i);
                    ++j;      
                }
            }
            //If joint was marked to be ignored, it shall be ignored.
            else{
                delta_thetas_(i) = 0;
            }

        }
        if( j == 0 ){
            std::cout << std::endl << "Robot will be unable to get out of this configuration using this controller.";
            delta_thetas_ = VectorXd::Zero(robot_dofs_);
            break;       
        }

        if(not should_break_loop){
            pseudo_robot = DQ_kinematics(step_dh_matrix);  //Change DH
            pseudo_robot.set_base(robot_base);
            pseudo_robot.set_effector(robot_effector);
            pseudo_thetas.conservativeResize(j);           //Resize pseudothetas
        }

        

    }//While not should_break_loop

    return delta_thetas_;

}





void Controller_invariant_Pid_SRIvar_Tracking::enableTrackingTerm(const bool bool_falseortrue)
{
    enable_trackintTerm = bool_falseortrue;
}



} // END OF NAMESPACE



     



