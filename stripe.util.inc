<?php

/**
 * Helper function to convert between cents and dollars.
 */
function stripe_transform_amount($amount) {
	if( (int)$amount == $amount) {
		$amount = (float)$amount / 100;
	} else {
		$amount = $amount * 100;
	}
	
	return $amount;
}

/**
 * Used for carts. Created whenever requested.
 */
function stripe_return_cart_id() {
	$cart_id = ($GLOBALS['user']->uid > 0) ? $GLOBALS['user']->uid : md5(uniqid(rand(), TRUE));
	
	if( ! db_result(db_query("SELECT cart_id FROM {stripe_cart} WHERE cart_id = '%s' LIMIT 1", $cart_id)) )
		db_query("INSERT INTO {stripe_cart} (cart_id) VALUES ('%s')", $cart_id);
	
	return $cart_id;
}

/**
 * Stores the amount in the stripe_cart table. There's no real cart at this time, in terms of multiple products.
 * In the future, amount will not be stored, but calculated.
 */
function stripe_set_amount($amount) {
	//$_SESSION['stripe_amount'] = (int)stripe_transform_amount((float)$amount);
	$cart_id = stripe_return_cart_id();
	
	db_query("UPDATE stripe_cart SET amount = '%f' WHERE cart_id = '%s'", (float)$amount, $cart_id);
}

/**
 * Returns a balance for the given cart_id.
 */
function stripe_get_balance($cart_id = '') {
	if( empty($cart_id) ) $cart_id = stripe_return_cart_id();
	
	return db_result(db_query("SELECT amount FROM {stripe_cart} WHERE cart_id = '%s'", $cart_id));
}

/**
 * Set the Stripe customer_id for a given uid.
 */
function stripe_set_customer_id($uid = 0, $data) {
	db_query("UPDATE {stripe_customers} SET customer_id = '%s' WHERE uid = %d", $customer_id, $uid);
	
	if(!db_affected_rows()) {
		db_query("INSERT INTO {stripe_customers} (uid, customer_id, name, card_number, card_type) VALUES (%d, '%s', '%s', %d, '%s')",
			$uid,
			$data['customer_id'],
			$data['name'],
			$data['card_number'],
			$data['card_type']);
	}
}

/**
 * Return the Stripe customer_id for a given uid.
 */
function stripe_get_customer_id($uid = 0) {
	if( ! $uid ) $uid = $GLOBALS['user']->uid;

	return db_fetch_object(db_query("SELECT * FROM {stripe_customers} WHERE uid = %d LIMIT 1", $uid));
}