/**
BAXTER DQ Control Example with joint limits consideration in control loop.

\author Luis Figueredo 
\since 07/2014

**/



#include "baxterdqcontrol.h"




//**** Setting robot to tracking mode ***
bool baxterdqcontrol::srvcallback_setctrl_idle(std_srvs::Empty::Request &request, std_srvs::Empty::Response &response)
{	
	FLAG_CONTROLLER_OPTION = 0;
	std::cout << std::endl << "======> [CONTROL]:[IDLE] Controller set to idle mode. Watch gravity making its thing!" << std::endl;
	return true;
}

//**** Setting robot to tracking mode ***
bool baxterdqcontrol::srvcallback_setctrl_setpoint(std_srvs::Empty::Request &request, std_srvs::Empty::Response &response)
{	
	FLAG_CONTROLLER_OPTION = 1;
	std::cout << std::endl << "======> [CONTROL]:[SET POINT] Controller set to user reference set point. Use topics /baxter_dqcontrol/left/control to set desired pose." << std::endl;
	return true;
}

//**** Setting robot to tracking mode ***
bool baxterdqcontrol::srvcallback_setctrl_tracking(std_srvs::Empty::Request &request, std_srvs::Empty::Response &response)
{	
	FLAG_CONTROLLER_OPTION = 2;
	std::cout << std::endl << "======> [CONTROL]:[TRACKING] Getting data from reference baxter arm and defining relative pose." << std::endl;	
	// Here we will try to obtain the data from reference controller. At the main loop,
	// ... we will check if we can read the data and set reference (FLAG..=3). If not, we will set idle mode (FLAG..=0).
	return true;
}



//************* new
void baxterdqcontrol::callbackInput_leftbutton(const baxter_core_msgs::DigitalIOState& button_state)
{	
	if (button_state.state==1)
	{		
		if (switching_time_button_left)
			std::cout << ".";
		else
		{
			std::cout << " receiving data from left button" << std::endl;
			if ( (FLAG_CONTROLLER_OPTION == 3) && (switching_time_button_left==false) )
			{
				FLAG_CONTROLLER_OPTION = 0;
				std::cout << std::endl << "======> [CONTROL]:[IDLE] Controller set to idle mode. Watch gravity making its thing!" << std::endl;
			}
			else
			{
				if ( (FLAG_CONTROLLER_OPTION == 0) && (switching_time_button_left==false) )
				{
					FLAG_CONTROLLER_OPTION = 2;
					std::cout << std::endl << "======> [CONTROL]: Getting data from reference baxter arm and defining relative pose." << std::endl;					
				}
			}
			switching_time_button_left = true;

		}
	}
	else
		switching_time_button_left = false;
}

void baxterdqcontrol::callbackInput_rightbutton(const baxter_core_msgs::DigitalIOState& button_state)
{
	if (button_state.state==1)
	{		
		if (switching_time_button_right)
			std::cout << ".";
		else
		{
			std::cout << " receiving data from right button" << std::endl;
			if ( (FLAG_CONTROLLER_OPTION == 3) && (switching_time_button_right==false) )
			{
				FLAG_CONTROLLER_OPTION = 0;
				std::cout << std::endl << "======> [CONTROL]:[IDLE] Controller set to idle mode. Watch gravity making its thing!" << std::endl;
			}
			else
			{
				if ( (FLAG_CONTROLLER_OPTION == 0) && (switching_time_button_right==false) )
				{
					FLAG_CONTROLLER_OPTION = 2;
					std::cout << std::endl << "======> [CONTROL]: Getting data from reference baxter arm and defining relative pose." << std::endl;					
				}
			}
			switching_time_button_right = true;

		}
	}
	else
		switching_time_button_right = false;	
}
		
		






/*#########################################################
####[ CLASS: baxterdqcontrol ]::[ TYPE: public function  ]
####========================================================
#### SET THE REFERENCE POSE FOR BAXTER ARM 
###########################################################	*/
void baxterdqcontrol::callbackInputReferencePose(const geometry_msgs::Pose& inputREF)
{
	// inputREF;
	dqvar_temp__dvec4[0] =  inputREF.orientation.x + inputREF.orientation.y + inputREF.orientation.z;
	dqvar_temp__dvec4[1] =  inputREF.orientation.x / dqvar_temp__dvec4[0];
	dqvar_temp__dvec4[2] =  inputREF.orientation.y / dqvar_temp__dvec4[0];
	dqvar_temp__dvec4[3] =  inputREF.orientation.z / dqvar_temp__dvec4[0];	
	if (inputREF.orientation.w == 0)
		dqvar_temp__dvec4[0] =  0.5*MATH_PI;
	else
		if (inputREF.orientation.w >=5)
			dqvar_temp__dvec4[0] =  0.5*(MATH_PI/180)*inputREF.orientation.w;
		else
			dqvar_temp__dvec4[0] =  0.5*inputREF.orientation.w;

	std::cout << "==> " << dqvar_temp__dvec4[0];	
	dqvar_dq_xd = DQ(cos(dqvar_temp__dvec4[0]), sin(dqvar_temp__dvec4[0])*dqvar_temp__dvec4[1], sin(dqvar_temp__dvec4[0])*dqvar_temp__dvec4[2], sin(dqvar_temp__dvec4[0])*dqvar_temp__dvec4[3], 0,0,0,0  );
	dqvar_dq_xd = dqvar_dq_xd*(dqvar_dq_xd.norm().inv());	
	dqvar_dq_xd = dqvar_dq_xd + 0.5*E_*DQ(0,inputREF.position.x,inputREF.position.y,inputREF.position.z,0,0,0,0)*dqvar_dq_xd;
	dqvar_dq_xd = dqvar_dq_xd*(dqvar_dq_xd.norm().inv());		

	// Print in screen new reference
	publishinformation(dqvar_dq_xd, true);
}





/*#########################################################
####[ CLASS: baxterdqcontrol ]::[ TYPE: public function  ]
####========================================================
#### SET THE REFERENCE POSITION FOR BAXTER (ignore or hold orientation)
###########################################################	*/
void baxterdqcontrol::callbackInputReferencePosition(const geometry_msgs::Point& inputREF)
{
	// obtain current orientation and hold that
	dqvar_temp__Mat8x1 = dqvar_dq_xm.vec8();
	dqvar_temp__dvec4[0] =  dqvar_temp__Mat8x1(0,0);		
	dqvar_temp__dvec4[1] =  dqvar_temp__Mat8x1(1,0);
	dqvar_temp__dvec4[2] =  dqvar_temp__Mat8x1(2,0);
	dqvar_temp__dvec4[3] =  dqvar_temp__Mat8x1(3,0);

	dqvar_dq_xd = DQ(dqvar_temp__dvec4[0], dqvar_temp__dvec4[1], dqvar_temp__dvec4[2], dqvar_temp__dvec4[3], 0,0,0,0  );
	dqvar_dq_xd = dqvar_dq_xd*(dqvar_dq_xd.norm().inv());	
	// Obtain reference position
	dqvar_dq_xd = dqvar_dq_xd + 0.5*E_*DQ(0,inputREF.x,inputREF.y,inputREF.z,0,0,0,0)*dqvar_dq_xd;
	dqvar_dq_xd = dqvar_dq_xd*(dqvar_dq_xd.norm().inv());	

	// Print in screen new reference
	publishinformation(dqvar_dq_xd, true);
}



void baxterdqcontrol::callbackInputReferencePosition_append(const geometry_msgs::Point& inputREF)
{
	// obtain current orientation and hold that
	dqvar_temp__Mat8x1 = dqvar_dq_xm.vec8();
	dqvar_temp__dvec4[0] =  dqvar_temp__Mat8x1(0,0);		
	dqvar_temp__dvec4[1] =  dqvar_temp__Mat8x1(1,0);
	dqvar_temp__dvec4[2] =  dqvar_temp__Mat8x1(2,0);
	dqvar_temp__dvec4[3] =  dqvar_temp__Mat8x1(3,0);
	dqvar_dq_xd = DQ(dqvar_temp__dvec4[0], dqvar_temp__dvec4[1], dqvar_temp__dvec4[2], dqvar_temp__dvec4[3], 0,0,0,0  );
	dqvar_dq_xd = dqvar_dq_xd*(dqvar_dq_xd.norm().inv());	

	// obtain current position and add value to the original position
	dqvar_dq_xm = dqvar_dq_xm*(dqvar_dq_xm.norm().inv());	
	dqvar_temp__Mat8x1 = dqvar_dq_xm.translation().vec8();
	dqvar_temp__dvec4[0] =  dqvar_temp__Mat8x1(1,0) + inputREF.x;		
	dqvar_temp__dvec4[1] =  dqvar_temp__Mat8x1(2,0) + inputREF.y;
	dqvar_temp__dvec4[2] =  dqvar_temp__Mat8x1(3,0) + inputREF.z;
	if (dqvar_temp__dvec4[2] > 0.4)
		dqvar_temp__dvec4[2]=0.38;
	if (dqvar_temp__dvec4[2] < -0.4)
		dqvar_temp__dvec4[2]= -0.38;	


	// Obtain reference position
	dqvar_dq_xd = dqvar_dq_xd + 0.5*E_*DQ(0,dqvar_temp__dvec4[0],dqvar_temp__dvec4[1],dqvar_temp__dvec4[2],0,0,0,0)*dqvar_dq_xd;
	dqvar_dq_xd = dqvar_dq_xd*(dqvar_dq_xd.norm().inv());	

	// Print in screen new reference
	publishinformation(dqvar_dq_xd, true);
}








/*#########################################################
####[ CLASS: baxterdqcontrol ]::[ TYPE: public function  ]
####========================================================
#### GET RELATIVE POSE FROM BOTH ARMS (It will be set as a constant pose)
###########################################################	*/
bool baxterdqcontrol::getConstantRelativePose(const Matrix<double,7,1>& argjoints)
{
	// GET CURRENT POSE AND RELATIVE ONE
	constlastjoint = argjoints(6,0);
	relativeinit_refjoints = argjoints;
	relativeinit_dqmain = dqvar_dq_xm;

	dqvar_dq_refxm = dqvar_ARM_KINE_REF.fkm(argjoints);    // UPDATE: BAXTER reference
	dqvar_dq_refxm = dqvar_dq_refxm*(dqvar_dq_refxm.norm().inv());	

	if (baxter_ros->FLAG_INIT__READING_LEFT_JOINTS==true) {				
		dqvar_dq_relpose = dqvar_dq_xm.conj()*dqvar_dq_refxm;	  // UPDATE: Relative pose 
		dqvar_dq_ref_initpose = dqvar_dq_refxm;
		dqvar_dq_ref_initpose = dqvar_dq_ref_initpose*(dqvar_dq_ref_initpose.norm().inv());	
		return true;
	} 		
	std::cout  << std::endl  << "[Warning]:[baxterdqcontrol]: Impossible to set relative pose. Data from current Robot (not the referece one) is unavailable." << std::endl ;
	return false;
}
	





/*#########################################################
####[ CLASS: baxterdqcontrol ]::[ TYPE: public function  ]
####========================================================
#### EXECUTION
###########################################################	*/

void baxterdqcontrol::execution()
{
	
	dqvar_dq_xd = DQ(1, 0, 0, 0, 0, 0, 0, 0);

	// *************  DEFINE CONTROLLER  *************  
	Controller_Generic controller(dqvar_ARM_KINE);
	controller.set_joint_limits(ctrlvar_joints__Mat7x1_LIMITS_UPPER, ctrlvar_joints__Mat7x1_LIMITS_LOWER);
	std::cout << "control gain: " <<  ctrlvar_pid__gain_kp << std::endl;
	controller.set_control_gains(1*ctrlvar_pid__gain_kp*ctrlvar_pid__Mat8x8_kp, ctrlvar_pid__gain_ki*ctrlvar_pid__Mat8x8_kp, 0.99, ctrlvar_pid__gain_kd*ctrlvar_pid__Mat8x8_kp);
	// controller.set_control_gains(ctrlvar_pid__gain_kp, ctrlvar_pid__gain_ki, 0.99, ctrlvar_pid__gain_kd);
	controller.set_jacob_srivar_paramconst(0.01, 0.05, 0.001);	

	ROS_INFO("[ OK ] DQ PID Controllers initialized.\n");

	int loopit = 41;
	int looptemp=0;
	
	// Publishing rate
	ros::Rate loop_rate(ctrlvar_time__dnum_CONTROL_FREQ);

	// Waiting to get data from BAXTER arm
	while (ros::ok()) {
		if (baxter_ros->FLAG_INIT__READING_LEFT_JOINTS==true)
			break;
		if (loopit++ > 40)	{
			ROS_INFO("[ Waiting ] Waiting data from main Robotic Manipulator...");
			loopit = 1;
		}
		ros::Duration(0.05).sleep();
	}

	// Initialize variables for control module
	loopit = 0;
	dqvar_dq_xm    = dqvar_ARM_KINE.fkm(baxter_ros->left_joints);


	ROS_INFO("[ Waiting ] Getting current pose...");
	dqvar_dq_xd = dqvar_dq_xm;
	ros::Duration(1.0).sleep();
	ROS_INFO("[ OK ] Control reference set to be equal current pose (regular start procedure)");
	std::cout << std::endl << "Reference:   " << dqvar_dq_xd;	
	std::cout << std::endl << "current pose:" << dqvar_dq_xm << std::endl;		


double oldteste;
oldteste = 0;
Matrix<double,7,1> relative_newinputjoints; 	

	//======================[ Control Loop ]============================
	while (ros::ok())
	{   		
		++loopit; 		
		//========================[ Check Poses  ]======================
		dqvar_dq_refxm = get_endeffector(baxter_ros->right_joints, false);
		dqvar_dq_xm = get_endeffector(baxter_ros->left_joints, true);

			if (enable_fixed_joint_test) {	
				relative_newinputjoints = baxter_ros->left_joints;
				relative_newinputjoints(6,0) = relativeinitjoint_main;
				dqvar_dq_xm = get_endeffector(relative_newinputjoints, true);
			}


		//========================[ Print Information  ]======================
		dqvar_dq_error = (1 - dqvar_dq_xm.conj()*dqvar_dq_xd);
		dqvar_norm__dnum_ERROR = dqvar_dq_error.vec8().norm();	
		if (FLAG__PRINT_DEBUG_INFORMATION)
		{
			publishinformation(dqvar_dq_xm, false);
			publishinformation_ref(dqvar_dq_refxm, false);
			publishinfo_error();		
		}	

		// CHECK TRACKING MODE AND OBTAINING THE REFERENCE DATA
		//====================================================================
		if (FLAG_CONTROLLER_OPTION == 2) {
			FLAG_CONTROLLER_OPTION = 3;				
			if (getConstantRelativePose(baxter_ros->right_joints))
				std::cout  << "======> [CONTROL]: Controller set to track reference Arm." << std::endl;		
			else {
				FLAG_CONTROLLER_OPTION = 0;
				std::cout  << "======> [CONTROL]:[Warning] Could not make track reference Arm work." << std::endl  << "======> [CONTROL]: Controller set to idle. Arm will hold pose" << std::endl;					
			}
			if (enable_fixed_joint_test)
				relativeinitjoint_main = baxter_ros->left_joints(6,0);
			continue;
		}		
		if (FLAG_CONTROLLER_OPTION == 0) {
			dqvar_dq_xd = dqvar_dq_xm;
			continue;
		}

		// ====================[  FEEDBACK IN CALLBACK  /OR/  WHILE-LOOP  ]====================
		if (baxter_ros->FLAG_INIT__READING_LEFT_JOINTS==true && FLAG_FEEDBACK_CONTROL_IN_CALLBACK==false )
		{
			//=========[  TRACKING MODE: adjust reference  ]========
			if (FLAG_CONTROLLER_OPTION == 3) { 
				dqvar_dq_refxm = get_endeffector(baxter_ros->right_joints, false);
				if (FLAG_TRACK_ONLY_POSITION==true) {
					dqvar_dq_refxm = dqvar_dq_refxm*(dqvar_dq_refxm.norm().inv());	
					dqvar_dq_xd = ( dqvar_dq_ref_initpose.P() + E_*(0.5*dqvar_dq_refxm.translation()*dqvar_dq_ref_initpose.P()) )*dqvar_dq_relpose.conj();

					dqvar_dq_xd = dqvar_dq_xd*(dqvar_dq_xd.norm().inv());			
				}
				else {
					if (enable_fixed_joint_test) {	

						relativeinit_refjoints(6,0) = relativenewjoint;
						relativerefpose =  dqvar_ARM_KINE_REF.fkm(relativeinit_refjoints);    
						relativerefpose = relativerefpose*(relativerefpose.norm().inv());	
						dqvar_dq_relpose = relativeinit_dqmain.conj()*relativerefpose;

						dqvar_dq_xd = dqvar_dq_refxm*dqvar_dq_relpose.conj();						
					}
					else
						dqvar_dq_xd = dqvar_dq_refxm*dqvar_dq_relpose.conj();
				}
			}
			


			// **** OBTAIN CONTROL OUTPUT **** 
			if (enable_fixed_joint_test)
				ctrlvar_joints__Mat7x1_output =  controller.getNewJointPositions(dqvar_dq_xd, relative_newinputjoints,1.0);
			else
				ctrlvar_joints__Mat7x1_output =  controller.getNewJointPositions(dqvar_dq_xd, baxter_ros->left_joints,1.0);
					
			// ****  SEND CONTROL OUTPUT  **** 
			if (enable_fixed_joint_test) {	
				oldteste = ctrlvar_joints__Mat7x1_output(6,0);
				ctrlvar_joints__Mat7x1_output(6,0) = relativeinitjoint_main + (relativenewjoint - constlastjoint);
				std::cout << " ===>  des: " << ctrlvar_joints__Mat7x1_output(6,0) << " x " << oldteste <<  "   xxxxx cur: " << baxter_ros->left_joints(6,0)  << std::endl; 
			}
			baxter_ros->publish_left_joints(ctrlvar_joints__Mat7x1_output);
		}

		// UNCOMMENT TO SET OUTPUT FUNCTION (small error = job done. single control law)
		// if ( dqvar_norm__dnum_ERROR < ctrlvar_norm__dnum_threshold)  {
		// 	std::cout << std::endl << "Error is small enought to get out of control loop" << std::endl  << std::endl;
		// 	break;
		// }
		// Sleep for the remaining time to let us hit our 10hz publishing rate
		loop_rate.sleep();	

	} // END-OF-WHILE
	//==========================================================================

}







/*#########################################################
####[ CLASS: baxterdqcontrol ]::[ TYPE: public function  ]
####========================================================
#### PUBLISH (PRINT) INFORMATION ABOUT POSE REFERENCE (in R3+quat) 
####      and Orientation (euler XYZ) + (rot_axis+angle)
###########################################################*/
void baxterdqcontrol::publishinformation_ref(DQ dq_state, bool printed)
{

	// Get Current Translation
	dqvar_temp__Mat8x1 = dq_state.translation().vec8();
	rosmsg_printpose_REF.position.x = dqvar_temp__Mat8x1(1,0);
	rosmsg_printpose_REF.position.y = dqvar_temp__Mat8x1(2,0);
	rosmsg_printpose_REF.position.z = dqvar_temp__Mat8x1(3,0);
	// Get Current First Quaternion
	dqvar_temp__Mat8x1 = dq_state.vec8();
	rosmsg_printpose_REF.orientation.x =  dqvar_temp__Mat8x1(1,0);
	rosmsg_printpose_REF.orientation.y =  dqvar_temp__Mat8x1(2,0);
	rosmsg_printpose_REF.orientation.z =  dqvar_temp__Mat8x1(3,0);
	rosmsg_printpose_REF.orientation.w =  dqvar_temp__Mat8x1(0,0);
	// Get Current Orientation
	dqvar_temp__Mat8x1 = dq_state.rot_axis().vec8();
	rosmsg_printrotation.orientation.x =  dqvar_temp__Mat8x1(1,0);
	rosmsg_printrotation.orientation.y =  dqvar_temp__Mat8x1(2,0);
	rosmsg_printrotation.orientation.z =  dqvar_temp__Mat8x1(3,0);
	rosmsg_printrotation.orientation.w =  dq_state.rot_angle();

	if (printed==true)
	{
		std::cout <<std::endl<<"     [dqRobotics-Control Info]"<<std::endl<<"========================================"<< std::endl ;
		std::cout <<"[dq]: " << dq_state << std::endl;
		std::cout <<"===[pose xyz]: " << rosmsg_printpose_REF.position.x <<"  "<<rosmsg_printpose_REF.position.y<<"  "<<rosmsg_printpose_REF.position.z << std::endl; 
		std::cout <<"===[rot][angle: "<< rosmsg_printrotation.orientation.w <<" ("<< rosmsg_printrotation.orientation.w*(180/MATH_PI) <<")]--[axis]: " << rosmsg_printrotation.orientation.x<<"  "<<rosmsg_printrotation.orientation.y<<"  "<<rosmsg_printrotation.orientation.z << std::endl<< std::endl; 
	}
	else
	{
		// Publish both information
			rosvar_pub__print_pose_right.publish(rosmsg_printpose_REF);
	}
}



/*#########################################################
####[ CLASS: baxterdqcontrol ]::[ TYPE: public function  ]
####========================================================
#### PUBLISH (PRINT) INFORMATION ABOUT POSE (in R3+quat) 
####      and Orientation (euler XYZ) + (rot_axis+angle)
###########################################################*/
void baxterdqcontrol::publishinformation(DQ dq_state, bool printed)
{

	// Get Current Translation
	dqvar_temp__Mat8x1 = dq_state.translation().vec8();
	rosmsg_printpose.position.x = dqvar_temp__Mat8x1(1,0);
	rosmsg_printpose.position.y = dqvar_temp__Mat8x1(2,0);
	rosmsg_printpose.position.z = dqvar_temp__Mat8x1(3,0);
	// Get Current First Quaternion
	dqvar_temp__Mat8x1 = dq_state.vec8();
	rosmsg_printpose.orientation.x =  dqvar_temp__Mat8x1(1,0);
	rosmsg_printpose.orientation.y =  dqvar_temp__Mat8x1(2,0);
	rosmsg_printpose.orientation.z =  dqvar_temp__Mat8x1(3,0);
	rosmsg_printpose.orientation.w =  dqvar_temp__Mat8x1(0,0);
	// Get Current Orientation
	dqvar_temp__Mat8x1 = dq_state.rot_axis().vec8();
	rosmsg_printrotation.orientation.x =  dqvar_temp__Mat8x1(1,0);
	rosmsg_printrotation.orientation.y =  dqvar_temp__Mat8x1(2,0);
	rosmsg_printrotation.orientation.z =  dqvar_temp__Mat8x1(3,0);
	rosmsg_printrotation.orientation.w =  dq_state.rot_angle();

	if (printed==true)
	{
		std::cout <<std::endl<<"     [dqRobotics-Control Info]"<<std::endl<<"========================================"<< std::endl ;
		std::cout <<"[dq]: " << dq_state << std::endl;
		std::cout <<"===[pose xyz]: " << rosmsg_printpose.position.x <<"  "<<rosmsg_printpose.position.y<<"  "<<rosmsg_printpose.position.z << std::endl; 
		std::cout <<"===[rot][angle: "<< rosmsg_printrotation.orientation.w <<" ("<< rosmsg_printrotation.orientation.w*(180/MATH_PI) <<")]--[axis]: " << rosmsg_printrotation.orientation.x<<"  "<<rosmsg_printrotation.orientation.y<<"  "<<rosmsg_printrotation.orientation.z << std::endl<< std::endl; 
	}
	else
	{
		// Publish both information
		if (FLAG_PUBLISH_INFO_DQ==true)
			rosvar_pub__print_pose_left.publish(rosmsg_printpose);
	}
}




/*#########################################################
####[ CLASS: baxterdqcontrol ]::[ TYPE: public function  ]
####========================================================
#### PUBLISH (PRINT) INFORMATION ABOUT CONTROL ERROR (no args)
###########################################################*/
void baxterdqcontrol::publishinfo_error()
{
	// Print info
	if (FLAG_PUBLISH_INFO_ERROR==true)			
	{
		rosaux_stringerror.str(" ");
		// rosaux_stringerror <<std::endl;
		rosaux_stringerror <<"     [dqRobotics-Control Output Info]" << std::endl ;
		rosaux_stringerror <<"========================================"<< std::endl;				
		rosaux_stringerror <<"===[Ref]: " << dqvar_dq_xd << std::endl; 
		rosaux_stringerror <<"===[Cur]: " << dqvar_dq_xm << std::endl;
		rosaux_stringerror <<"===[Error]: " << dqvar_norm__dnum_ERROR  << std::endl;		
		//rosaux_stringerror <<"========================================"<< std::endl;
		rosmsg_printerror.data = rosaux_stringerror.str();
		// Publishes string:
		rosvar_pub__print_ERROR.publish(rosmsg_printerror);
	}
}













/*#########################################################
####[ CLASS: baxterdqcontrol ]::[ TYPE: public function  ]
####========================================================
#### INIT (CONFIG) DATA FOR THE ROS
###########################################################*/
void baxterdqcontrol::init_ROS_Config()
{

	std::cout << std::endl;
	ROS_INFO("[ OK ] Start dqnode to control Baxter arm");   

	// ********** VERIFY IF ANY DATA HAS ALREADY BEEN RECEIVED: **********
	FLAG_CONTROLLER_OPTION = 0;
	rosvar_pub__print_pose_right = rosnode.advertise<geometry_msgs::Pose>("/baxter_dqcontrol/right/pose",1000);		
	rosvar_pub__print_pose_left = rosnode.advertise<geometry_msgs::Pose>("/baxter_dqcontrol/left/pose",1000);	
	rosvar_pub__print_ERROR = rosnode.advertise<std_msgs::String>("/baxter_dqcontrol/left/error_info",1000);		

//**********[  Controlling the buttons ]		
		rosvar_sub__get_left_button = rosnode.subscribe("/robot/digital_io/left_itb_button0/state",1000,&baxterdqcontrol::callbackInput_leftbutton, this);
		rosvar_sub__get_right_button = rosnode.subscribe("/robot/digital_io/right_itb_button0/state",1000,&baxterdqcontrol::callbackInput_rightbutton, this);

	rosvar_subs__get_ref_pose = rosnode.subscribe("/baxter_dqcontrol/left/control/pose",1000,&baxterdqcontrol::callbackInputReferencePose, this);
	rosvar_subs__get_ref_position = rosnode.subscribe("/baxter_dqcontrol/left/control/position",1000,&baxterdqcontrol::callbackInputReferencePosition,this);
	rosvar_subs__get_ref_position_appended = rosnode.subscribe("/baxter_dqcontrol/left/control/add_translation",1000,&baxterdqcontrol::callbackInputReferencePosition_append,this);

	// 
	rossrv_changeCtrl_idle     = rosnode.advertiseService("/baxter_dqcontrol/set_control/idle", &baxterdqcontrol::srvcallback_setctrl_idle,this);
	rossrv_changeCtrl_setpoint = rosnode.advertiseService("/baxter_dqcontrol/set_control/point", &baxterdqcontrol::srvcallback_setctrl_setpoint,this);
	rossrv_changeCtrl_tracking = rosnode.advertiseService("/baxter_dqcontrol/set_control/track", &baxterdqcontrol::srvcallback_setctrl_tracking,this);	

	// Initialize msg variables		
	rosmsg_printerror.data = " ";
}





/*#########################################################
####[ CLASS: baxterdqcontrol ]::[ TYPE: public function  ]
####========================================================
#### INIT (CONFIG) DATA FOR THE ROBOTIC ARM
###########################################################	*/
void baxterdqcontrol::init_ROBOT_ARM_CONFIG()
{

	// Define robot DOF (just for checking, the program is mostly set to 7 DOF by now)
	ctrlvar_robotnumberofdof =7;	
	relativeinit_refjoints << 0,0,0,0,0,0,0;
	relativeinit_dqmain = DQ(1);
		
	// GAIN MATRIX
	ctrlvar_pid__Mat8x8_kp = Matrix<double,8,8>::Zero(8,8);
	Matrix<double,8,1> kp_diagonal(8,1);
	kp_diagonal << 1,1,1,1,1,1,1,1;
	ctrlvar_pid__Mat8x8_kp.diagonal() = kp_diagonal;

 	//Robot DH
	dqvar_ARM_KINE = BaxterArm_Kinematics();
	dqvar_ARM_KINE.set_base( dqvar_dq_base );

	dqvar_ARM_KINE_REF =  BaxterArm_Kinematics();
	dqvar_ARM_KINE_REF.set_base( dqvar_dq_refbase);


	//INIT Control Loop Variables
	dqvar_dq_xm = DQ(0);
	dqvar_dq_error = DQ(2); //An initial large value so it does not break from the control loop
	ctrlvar_norm__dnum_threshold = 1.e-5;
	ctrlvar_counter__ctrl_step=0;

	//=======================================================
	// BAXTER LIMITS ( "s0", "s1", "e0", "e1", "w0","w1", "w2"  ):
	ctrlvar_joints__Mat7x1_LIMITS_UPPER <<  1.70167993878,  1.047,  3.05417993878,  2.618,  3.059,  2.094, 3.059;
	//**** Right values:
	// ctrlvar_joints__Mat7x1_LIMITS_LOWER << -1.70167993878, -1.410, -3.05417993878, -0.05, -3.059, -1.57079632679, -3.059;
	//**** Just to make sure that it doesnt go to close
	ctrlvar_joints__Mat7x1_LIMITS_LOWER << -1.20167993878, -1.410, -3.05417993878, -0.05, -3.059, -1.57079632679, -3.059;
	//                    joint 2 gets sometimes to -1.54 but its not to trust this vlue
	//=======================================================

}



DQ baxterdqcontrol::get_endeffector(const Matrix<double,7,1>& joints, bool main_arm)
{		
	DQ dqOutput;
	// matrixxd vectorJoints;
	// vectorJoints = joints;	

	// if (FLAG_CONTROLLER_OPTION == 3) 
	// 	vectorJoints(6,0) = constlastjoint;

	if (main_arm) 
		dqOutput = dqvar_ARM_KINE.fkm( joints );  
	else
	{
		Matrix<double,7,1>  temp_joints; 
		temp_joints = joints;		
		if (enable_fixed_joint_test) {			
			// temp_joints(6,0) = constlastjoint;
			relativenewjoint = joints(6,0);
		}
		dqOutput = dqvar_ARM_KINE_REF.fkm( temp_joints );  						
	}
	dqOutput = dqOutput*(  dqOutput.norm().inv()  );	
	return dqOutput;
}


//}; // END OF CLASS



















int main(int argc, char **argv)
{
	// Init node
	ros::init(argc, argv, "baxter_dq_control",  ros::init_options::AnonymousName);

	// Init Baxter Communication Node
	baxter_comm baxter_ros_init(argc, argv);

	// Execution procedure
	baxterdqcontrol dqbaxterarm_instance;
    dqbaxterarm_instance.init_setConstantValues(baxter_ros_init);	
	dqbaxterarm_instance.init_ROS_Config();
	dqbaxterarm_instance.init_ROBOT_ARM_CONFIG();	

	ros::AsyncSpinner spinner(4);
	spinner.start();

	dqbaxterarm_instance.execution();


    return 0;


}



